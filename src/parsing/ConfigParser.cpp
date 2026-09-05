#include "../../webserv.hpp"


ConfigParser::ConfigParser(const std::string &filename)
    : _filename(filename), _pos(0)
{
}
bool ConfigParser::tokenize()
{
    int fd = open(_filename.c_str(), O_RDONLY);
    if (fd < 0)
    {
        error("Cannot open config file: " + _filename);
        return false;
    }

    std::string content;
    char        buf[4096];
    ssize_t     bytes;

    while ((bytes = read(fd, buf, sizeof(buf))) > 0)
        content.append(buf, static_cast<size_t>(bytes));

    close(fd);

    if (bytes < 0)
    {
        error("Error reading config file: " + _filename);
        return false;
    }

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line))
    {
        size_t comment = line.find('#');
        if (comment != std::string::npos)
            line = line.substr(0, comment);

        std::istringstream iss(line);
        std::string word;

        while (iss >> word)
        {
            std::string buf2;

            for (size_t i = 0; i < word.size(); i++)
            {
                char c = word[i];

                if (c == '{' || c == '}' || c == ';')
                {
                    if (!buf2.empty())
                    {
                        _tokens.push_back(buf2);
                        buf2.clear();
                    }
                    _tokens.push_back(std::string(1, c));
                }
                else
                {
                    buf2 += c;
                }
            }

            if (!buf2.empty())
                _tokens.push_back(buf2);
        }
    }

    return true;
}

const std::string &ConfigParser::current() const
{
    if (_pos >= _tokens.size())
    {
        static const std::string empty;
        return empty;
    }
    return _tokens[_pos];
}

std::string ConfigParser::consume()
{
    return _tokens[_pos++];
}

bool ConfigParser::isEnd() const
{
    return _pos >= _tokens.size();
}

bool ConfigParser::expect(const std::string &expected)
{
    if (isEnd())
    {
        error("Expected '" + expected + "' but reached end of file");
        return false;
    }
    std::string token = consume();
    if (token != expected)
    {
        error("Expected '" + expected + "' but got '" + token + "'");
        return false;
    }
    return true;
}


size_t ConfigParser::parseSize(const std::string &str) const
{
    if (str.empty())
        return 0;

    char suffix = str[str.size() - 1];

    std::string num_str;
    if (suffix == 'K' || suffix == 'k' ||
        suffix == 'M' || suffix == 'm' ||
        suffix == 'G' || suffix == 'g')
    {
        num_str = str.substr(0, str.size() - 1);
    }
    else
    {
        num_str = str;
        suffix = 'B';
    }

    size_t value = static_cast<size_t>(std::atoi(num_str.c_str()));

    if (suffix == 'K' || suffix == 'k') return value * 1024;
    if (suffix == 'M' || suffix == 'm') return value * 1024 * 1024;
    if (suffix == 'G' || suffix == 'g') return value * 1024 * 1024 * 1024;
    return value;
}

void ConfigParser::error(const std::string &msg) const
{
    std::cerr << "[ConfigParser] Error: " << msg << std::endl;
}

const std::vector<ServerConfig> &ConfigParser::getServers() const
{
    return _servers;
}


bool ConfigParser::parse()
{
    if (!tokenize())
        return false;

    if (_tokens.empty())
    {
        error("Config file is empty");
        return false;
    }

    while (!isEnd())
    {
        std::string token = consume();

        if (token == "server")
        {
            if (!expect("{"))
                return false;

            ServerConfig server;

            if (!parseServer(server))
                return false;

            for (size_t i = 0; i < server.ports.size(); i++)
            {
                if (server.ports[i] <= 0 || server.ports[i] > 65535)
                {
                    error("Invalid or missing port in server block");
                    return false;
                }
            }

            _servers.push_back(server);
        }
        else
        {
            error("Unexpected token at top level: '" + token + "'");
            return false;
        }
    }

    if (_servers.empty())
    {
        error("No server blocks found in config file");
        return false;
    }

    return true;
}


bool ConfigParser::parseServer(ServerConfig &server)
{
    while (!isEnd())
    {
        std::string token = consume();

        if (token == "}")
            return true;

        else if (token == "listen")
        {
            if (isEnd()) { error("Missing value after 'listen'"); return false; }

            std::string value = consume();
            size_t colon = value.find(':');
            
            int port;
            if (colon != std::string::npos)
            {
                server.host = value.substr(0, colon);
                port = std::atoi(value.substr(colon + 1).c_str());
            }
            else
            {
                port = std::atoi(value.c_str());
            }
            
            server.ports.push_back(port);
            if (!expect(";")) return false;
        }
        else if (token == "server_name")
        {
            if (isEnd()) { error("Missing value after 'server_name'"); return false; }
            server.server_name = consume();
            if (!expect(";")) return false;
        }
        else if (token == "root")
        {
            if (isEnd()) { error("Missing value after 'root'"); return false; }
            server.root = consume();
            if (!expect(";")) return false;
        }
        else if (token == "index")
        {
            if (isEnd()) { error("Missing value after 'index'"); return false; }
            server.index = consume();
            if (!expect(";")) return false;
        }
        else if (token == "client_max_body_size")
        {
            if (isEnd()) { error("Missing value after 'client_max_body_size'"); return false; }
            server.client_max_body_size = parseSize(consume());
            if (!expect(";")) return false;
        }
        else if (token == "error_page")
        {
            if (isEnd()) { error("Missing error code after 'error_page'"); return false; }

            std::vector<int> codes;

            while (!isEnd())
            {
                const std::string &tok = current();
                bool isNumeric = !tok.empty();
                for (size_t i = 0; i < tok.size(); i++)
                {
                    if (!std::isdigit(static_cast<unsigned char>(tok[i])))
                    {
                        isNumeric = false;
                        break;
                    }
                }
                if (!isNumeric)
                    break;

                int code = std::atoi(consume().c_str());
                if (code <= 0 || code < 400 || code >= 600)
                {
                    error("Invalid error code (must be 400-599)");
                    return false;
                }
                codes.push_back(code);
            }

            if (codes.empty())
            {
                error("error_page requires at least one error code");
                return false;
            }

            if (isEnd()) { error("Missing path after error code(s)"); return false; }

            std::string path = consume();
            for (size_t i = 0; i < codes.size(); i++)
                server.error_pages[codes[i]] = path;

            if (!expect(";")) return false;
        }
        else if (token == "location")
        {
            if (isEnd()) { error("Missing path after 'location'"); return false; }

            LocationConfig location;
            location.path = consume();

            if (!expect("{")) return false;

            if (!parseLocation(server, location))
                return false;

            if (location.root.empty())
                location.root = server.root;

            if (location.index.empty())
                location.index = server.index;

            server.locations.push_back(location);
        }
        else
        {
            error("Unknown directive in server block: '" + token + "'");
            return false;
        }
    }

    error("Unexpected end of file — missing '}' for server block");
    return false;
}

bool ConfigParser::parseLocation(ServerConfig &server, LocationConfig &location)
{
    (void)server;

    while (!isEnd())
    {
        std::string token = consume();

        if (token == "}")
            return true;

        else if (token == "root")
        {
            if (isEnd()) { error("Missing value after 'root'"); return false; }
            location.root = consume();
            location.has_explicit_root = true;
            if (!expect(";")) return false;
        }
        else if (token == "index")
        {
            if (isEnd()) { error("Missing value after 'index'"); return false; }
            location.index = consume();
            location.has_explicit_index = true;
            if (!expect(";")) return false;
        }
        else if (token == "autoindex")
        {
            if (isEnd()) { error("Missing value after 'autoindex'"); return false; }
            std::string val = consume();
            if (val != "on" && val != "off")
            {
                error("'autoindex' must be 'on' or 'off', got: '" + val + "'");
                return false;
            }
            location.autoindex = (val == "on");
            if (!expect(";")) return false;
        }
        else if (token == "limit_except")
        {
            location.allowed_methods.clear();

            while (!isEnd() && current() != ";")
            {
                std::string method = consume();
                if (method != "GET" && method != "POST" && method != "DELETE" && method != "HEAD")
                {
                    error("Unknown method in 'limit_except': '" + method + "'");
                    return false;
                }
                location.allowed_methods.push_back(method);
            }

            if (location.allowed_methods.empty())
            {
                error("'limit_except' requires at least one method");
                return false;
            }

            if (!expect(";")) return false;
        }
        else if (token == "return")
        {
            if (isEnd()) { error("Missing value after 'return'"); return false; }

            const std::string &first = current();
            bool looksLikeCode = !first.empty() &&
                std::isdigit(static_cast<unsigned char>(first[0]));

            if (looksLikeCode)
            {
                location.redirect_code = std::atoi(consume().c_str());
                if (location.redirect_code != 301 && location.redirect_code != 302)
                {
                    error("'return' code must be 301 or 302");
                    return false;
                }
                if (isEnd()) { error("Missing URL after redirect code"); return false; }
                location.redirect = consume();
            }
            else
            {
                location.redirect_code = 302;
                location.redirect = consume();
            }

            if (!expect(";")) return false;
        }
        else if (token == "upload_store")
        {
            if (isEnd()) { error("Missing path after 'upload_store'"); return false; }
            location.upload_store = consume();
            if (!expect(";")) return false;
        }
        else if (token == "upload_path")
        {
            if (isEnd()) { error("Missing path after 'upload_path'"); return false; }
            location.upload_path = consume();
            if (!expect(";")) return false;
        }
        else if (token == "allow_upload")
        {
            if (isEnd()) { error("Missing value after 'allow_upload'"); return false; }
            std::string val = consume();
            if (val != "on" && val != "off")
            {
                error("'allow_upload' must be 'on' or 'off', got: '" + val + "'");
                return false;
            }
            location.allow_upload = (val == "on");
            if (!expect(";")) return false;
        }
        else if (token == "allow_delete")
        {
            if (isEnd()) { error("Missing value after 'allow_delete'"); return false; }
            std::string val = consume();
            if (val != "on" && val != "off")
            {
                error("'allow_delete' must be 'on' or 'off', got: '" + val + "'");
                return false;
            }
            location.allow_delete = (val == "on");
            if (!expect(";")) return false;
        }
        else if (token == "cgi_pass")
        {
            if (isEnd()) { error("Missing extension after 'cgi_pass'"); return false; }
            std::string ext = consume();

            if (ext.empty() || ext[0] != '.')
            {
                error("CGI extension must start with '.', got: '" + ext + "'");
                return false;
            }

            if (isEnd()) { error("Missing executable path after CGI extension"); return false; }
            std::string exec_path = consume();

            location.cgi_pass[ext] = exec_path;
            if (!expect(";")) return false;
        }
        else if (token == "client_max_body_size")
        {
            if (isEnd()) { error("Missing value after 'client_max_body_size'"); return false; }
            location.client_max_body_size = parseSize(consume());
            location.has_client_max_body_size = true;
            if (!expect(";")) return false;
        }
        else if (token == "internal")
        {
            location.internal = true;
            if (!expect(";")) return false;
        }
        else
        {
            error("Unknown directive in location block: '" + token + "'");
            return false;
        }
    }

    error("Unexpected end of file — missing '}' for location block");
    return false;
}
