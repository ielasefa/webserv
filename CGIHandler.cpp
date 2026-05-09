#include "webserv.hpp"
static const int CGI_TIMEOUT = 10;

CGIHandler::CGIHandler(const HttpRequest  &request,
                       const std::string  &script_path,
                       const std::string  &cgi_executable)
    : _request(request),
      _script_path(script_path),
      _cgi_executable(cgi_executable)
{}

std::string CGIHandler::unchunkBody(const std::string &chunked) const
{
    std::string result;
    size_t pos = 0;

    while (pos < chunked.size())
    {
        size_t line_end = chunked.find("\r\n", pos);
        if (line_end == std::string::npos)
            break;

        std::string size_line = chunked.substr(pos, line_end - pos);

        size_t semi = size_line.find(';');
        if (semi != std::string::npos)
            size_line = size_line.substr(0, semi);

        size_t chunk_size = 0;
        std::istringstream hex_stream(size_line);
        hex_stream >> std::hex >> chunk_size;

        if (chunk_size == 0)
            break;

        pos = line_end + 2;

        if (pos + chunk_size > chunked.size())
            break;

        result += chunked.substr(pos, chunk_size);

        pos += chunk_size + 2;
    }

    return result;
}

char **CGIHandler::buildEnv() const
{
    std::vector<std::string> vars;
    vars.push_back("REQUEST_METHOD=" + _request.method);

    vars.push_back("QUERY_STRING=" + _request.query_string);

    vars.push_back("SCRIPT_FILENAME=" + _script_path);
    vars.push_back("SCRIPT_NAME=" + _request.path);

    vars.push_back("PATH_INFO=" + _request.path);

    vars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    vars.push_back("SERVER_SOFTWARE=webserv/1.0");

    if (_request.headers.count("Content-Type"))
        vars.push_back("CONTENT_TYPE=" + _request.headers.at("Content-Type"));
    else
        vars.push_back("CONTENT_TYPE=");

    if (_request.headers.count("Content-Length"))
    {
        vars.push_back("CONTENT_LENGTH=" + _request.headers.at("Content-Length"));
    }
    else if (!_request.body.empty())
    {
        std::ostringstream oss;
        oss << _request.body.size();
        vars.push_back("CONTENT_LENGTH=" + oss.str());
    }
    else
    {
        vars.push_back("CONTENT_LENGTH=0");
    }

    if (_request.headers.count("Cookie"))
        vars.push_back("HTTP_COOKIE=" + _request.headers.at("Cookie"));

    if (_request.headers.count("Accept"))
        vars.push_back("HTTP_ACCEPT=" + _request.headers.at("Accept"));

    if (_request.headers.count("User-Agent"))
        vars.push_back("HTTP_USER_AGENT=" + _request.headers.at("User-Agent"));

    if (_request.headers.count("Host"))
        vars.push_back("HTTP_HOST=" + _request.headers.at("Host"));

    char **env = new char*[vars.size() + 1];

    for (size_t i = 0; i < vars.size(); i++)
    {
        env[i] = strdup(vars[i].c_str());
    }

    env[vars.size()] = NULL;

    return env;
}

void CGIHandler::freeEnv(char **env) const
{
    if (!env) return;

    for (int i = 0; env[i] != NULL; i++)
        free(env[i]);

    delete[] env;
}

std::string CGIHandler::wrapResponse(const std::string &cgi_output) const
{
    bool has_headers = (cgi_output.find("\r\n\r\n") != std::string::npos ||
                        cgi_output.find("\n\n")     != std::string::npos);

    if (has_headers)
    {
        return "HTTP/1.1 200 OK\r\n" + cgi_output;
    }
    else
    {
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << cgi_output.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << cgi_output;
        return response.str();
    }
}

std::string CGIHandler::execute()
{
    struct stat script_stat;
    if (stat(_script_path.c_str(), &script_stat) != 0)
    {
        std::cerr << "[CGI] Script not found: " << _script_path << std::endl;
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 22\r\n\r\n<h1>404 Not Found</h1>";
    }

    std::string body = _request.body;

    if (_request.headers.count("Transfer-Encoding"))
    {
        const std::string &te = _request.headers.at("Transfer-Encoding");
        if (te.find("chunked") != std::string::npos)
            body = unchunkBody(body);
    }

    int pipe_in[2];
    int pipe_out[2];

    if (pipe(pipe_in) < 0)
    {
        std::cerr << "[CGI] pipe() failed for stdin" << std::endl;
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
    }
    if (pipe(pipe_out) < 0)
    {
        std::cerr << "[CGI] pipe() failed for stdout" << std::endl;
        close(pipe_in[0]);
        close(pipe_in[1]);
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
    }

    char **env = buildEnv();

    pid_t pid = fork();

    if (pid < 0)
    {
        std::cerr << "[CGI] fork() failed" << std::endl;
        freeEnv(env);
        close(pipe_in[0]);  close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
    }

    if (pid == 0)
    {
        if (dup2(pipe_in[0], STDIN_FILENO) < 0)
        {
            std::cerr << "[CGI] dup2 stdin failed" << std::endl;
            close(pipe_in[0]);  close(pipe_in[1]);
            close(pipe_out[0]); close(pipe_out[1]);
            freeEnv(env);
            exit(1);
        }

        if (dup2(pipe_out[1], STDOUT_FILENO) < 0)
        {
            std::cerr << "[CGI] dup2 stdout failed" << std::endl;
            close(pipe_in[0]);  close(pipe_in[1]);
            close(pipe_out[0]); close(pipe_out[1]);
            freeEnv(env);
            exit(1);
        }

        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);

        std::string dir = _script_path.substr(0, _script_path.rfind('/'));
        if (!dir.empty() && chdir(dir.c_str()) < 0)
        {
            std::cerr << "[CGI] chdir failed: " << dir << std::endl;
            freeEnv(env);
            exit(1);
        }

        char *argv[3];
        argv[0] = strdup(_cgi_executable.c_str());
        argv[1] = strdup(_script_path.c_str());
        argv[2] = NULL;

        if (!argv[0] || !argv[1])
        {
            free(argv[0]);
            free(argv[1]);
            freeEnv(env);
            exit(1);
        }

        execve(_cgi_executable.c_str(), argv, env);

        std::cerr << "[CGI] execve failed: " << _cgi_executable << std::endl;
        free(argv[0]);
        free(argv[1]);
        freeEnv(env);
        exit(1);
    }

    freeEnv(env);

    close(pipe_in[0]);
    close(pipe_out[1]);

    if (!body.empty())
    {
        const char *ptr = body.c_str();
        size_t total = body.size();
        size_t sent = 0;

        while (sent < total)
        {
            ssize_t n = write(pipe_in[1], ptr + sent, total - sent);
            if (n < 0)
            {
                std::cerr << "[CGI] write to pipe_in failed" << std::endl;
                break;
            }
            sent += static_cast<size_t>(n);
        }
    }
    close(pipe_in[1]);

    std::string cgi_output;
    char buffer[4096];
    ssize_t bytes;
    bool timed_out = false;

    struct timeval start, now;
    gettimeofday(&start, NULL);

    while (true)
    {
        struct pollfd pfd;
        pfd.fd = pipe_out[0];
        pfd.events = POLLIN;
        pfd.revents = 0;

        int ready = poll(&pfd, 1, 100);

        if (ready < 0)
            break;

        if (ready > 0 && (pfd.revents & POLLIN))
        {
            bytes = read(pipe_out[0], buffer, sizeof(buffer));
            if (bytes <= 0)
                break;
            cgi_output.append(buffer, bytes);
        }

        gettimeofday(&now, NULL);
        long elapsed = (now.tv_sec - start.tv_sec);
        if (elapsed >= CGI_TIMEOUT_S)
        {
            std::cerr << "[CGI] Timeout after " << CGI_TIMEOUT_S
                      << "s — killing child " << pid << std::endl;
            kill(pid, SIGKILL);
            timed_out = true;
            break;
        }
    }
    close(pipe_out[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    if (timed_out)
    {
        return "HTTP/1.1 504 Gateway Timeout\r\n"
               "Content-Length: 26\r\n\r\n"
               "<h1>504 CGI Timed Out</h1>";
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    {
        std::cerr << "[CGI] Script exited with code "
                  << WEXITSTATUS(status) << std::endl;
        return "HTTP/1.1 500 Internal Server Error\r\n"
               "Content-Length: 27\r\n\r\n"
               "<h1>CGI execution failed</h1>";
    }

    if (cgi_output.empty())
        return "HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n";

    return wrapResponse(cgi_output);
}
