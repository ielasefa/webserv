#ifndef PARSING_CONFIG_HPP
#define PARSING_CONFIG_HPP

#include <string>       
#include <vector>       
#include <map>          
#include <iostream>     
#include <fstream>      
#include <sstream>      
#include <algorithm>    
#include <cstdlib>      
#include <cstring>      
#include <cerrno>       
#include <unistd.h>     
#include <fcntl.h>          
#include <poll.h>           
#include <signal.h>         
#include <sys/socket.h>     
#include <sys/types.h>      
#include <sys/stat.h>       
#include <sys/wait.h>       
#include <sys/time.h>       
#include <netinet/in.h>     
#include <arpa/inet.h>      
#include <dirent.h>         
extern const int    BACKLOG;
extern const int    POLL_TIMEOUT_MS;
extern const int    CGI_TIMEOUT_S;
extern const char  *DEFAULT_CONFIG_PATH;
extern const int    READ_BUFFER_SIZE;

struct HttpRequest
{
    std::string method;

    std::string path;

    std::string query_string;

    std::string version;

    std::map<std::string, std::string> headers;

    std::string body;
};

struct HttpResponse
{
    int status_code;

    std::string status_text;

    std::map<std::string, std::string> headers;

    std::string body;

    HttpResponse() : status_code(200), status_text("OK") {}

    std::string toString() const
    {
        std::ostringstream out;
        out << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";

        std::map<std::string, std::string>::const_iterator it;
        for (it = headers.begin(); it != headers.end(); ++it)
            out << it->first << ": " << it->second << "\r\n";

        if (headers.find("Content-Length") == headers.end())
        {
            std::ostringstream len;
            len << body.size();
            out << "Content-Length: " << len.str() << "\r\n";
        }

        out << "Connection: close\r\n";
        out << "\r\n";
        out << body;
        return out.str();
    }
};

class LocationConfig
{
public:
    std::string path;

    std::string root;

    std::string index;

    std::vector<std::string> allowed_methods;

    bool autoindex;

    std::string upload_store;

    int         redirect_code;
    std::string redirect;

    std::map<std::string, std::string> cgi_pass;
    
    bool        internal;

    bool        allow_upload;
    std::string upload_path;
    bool        allow_delete;

    LocationConfig();
};

class ServerConfig
{
public:
    std::string host;

    int port;

    std::string server_name;

    std::string root;

    std::string index;

    size_t client_max_body_size;

    std::map<int, std::string> error_pages;

    std::vector<LocationConfig> locations;

    LocationConfig matchLocation(const std::string& path) const;


    ServerConfig();
};

class ConfigParser
{
public:
    explicit ConfigParser(const std::string &filename);

    bool parse();

    const std::vector<ServerConfig> &getServers() const;

private:

    std::string               _filename;
    std::vector<std::string>  _tokens;
    size_t                    _pos;
    std::vector<ServerConfig> _servers;

    bool tokenize();

    bool parseServer(ServerConfig &server);

    bool parseLocation(ServerConfig &server, LocationConfig &location);

    const std::string &current() const;
    std::string        consume();
    bool               isEnd()   const;
    bool               expect(const std::string &expected);

    size_t parseSize(const std::string &str) const;
    void   error(const std::string &msg)    const;
};

class CGIHandler
{
public:

    CGIHandler(const HttpRequest &request,
               const std::string &script_path,
               const std::string &cgi_executable);

    std::string execute();

private:

    HttpRequest _request;
    std::string _script_path;
    std::string _cgi_executable;

    char      **buildEnv()                                  const;
    void        freeEnv(char **env)                         const;
    std::string unchunkBody(const std::string &chunked)     const;
    std::string wrapResponse(const std::string &cgi_output) const;
};

int     init_socket(const ServerConfig& serv);
void    multiplexing(int sfd, const ServerConfig& serv);

#endif // PARSING_CONFIG_HPP
