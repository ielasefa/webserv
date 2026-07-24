#include "webserv.hpp"

<<<<<<< HEAD:src/parsing/CGIHandler.cpp
// static const int UNUSED_CGI_TIMEOUT = 10;
=======
// static const int unsed cgi timeout = 10;
>>>>>>> jaafar:CGIHandler.cpp

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

std::string CGIHandler::getBody() const
{
    return _request.body;
}

bool CGIHandler::startCGI(int &fd_in, int &fd_out, pid_t &pid)
{
    struct stat script_stat;
    if (stat(_script_path.c_str(), &script_stat) != 0)
    {
        std::cerr << "[CGI] Script not found: " << _script_path << std::endl;
        return false;
    }

    // Process chunked body before handing it over
    if (_request.headers.count("Transfer-Encoding"))
    {
        const std::string &te = _request.headers.at("Transfer-Encoding");
        if (te.find("chunked") != std::string::npos)
            _request.body = unchunkBody(_request.body);
    }

    int pipe_in[2];
    int pipe_out[2];

    if (pipe(pipe_in) < 0 || pipe(pipe_out) < 0)
    {
        std::cerr << "[CGI] pipe() failed" << std::endl;
        return false;
    }

    // Set pipes to non-blocking
    fcntl(pipe_in[1], F_SETFL, O_NONBLOCK);
    fcntl(pipe_out[0], F_SETFL, O_NONBLOCK);

    char **env = buildEnv();
    pid = fork();

    if (pid < 0)
    {
        std::cerr << "[CGI] fork() failed" << std::endl;
        freeEnv(env);
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        return false;
    }

    if (pid == 0) // Child Process
    {
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);

        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);

        std::string dir = _script_path.substr(0, _script_path.rfind('/'));
        if (!dir.empty() && chdir(dir.c_str()) < 0)
        {
            freeEnv(env);
            exit(1);
        }

        char *argv[3];
        argv[0] = strdup(_cgi_executable.c_str());
        argv[1] = strdup(_script_path.c_str());
        argv[2] = NULL;

        execve(_cgi_executable.c_str(), argv, env);
        
        //if the excuve fails
        free(argv[0]);
        free(argv[1]);
        freeEnv(env);
        exit(1);
    }

    // Parent Process
    freeEnv(env);
    
    close(pipe_in[0]);
    close(pipe_out[1]);

    // Give these fd to the main server loop
    fd_in = pipe_in[1];   
    fd_out = pipe_out[0]; 

    return true;
}