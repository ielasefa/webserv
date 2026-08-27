#ifndef WEBSERV_HPP
#define WEBSERV_HPP

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
#include <sys/epoll.h>      
#include <netinet/in.h>     
#include <arpa/inet.h>      
#include <dirent.h>         
#include <time.h>

extern const int    BACKLOG;
extern const int    POLL_TIMEOUT_MS;
extern const int    CGI_TIMEOUT_S;
extern const char  *DEFAULT_CONFIG_PATH;
extern const int    READ_BUFFER_SIZE;

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string host;
    std::map<std::string, std::string> headers;

    std::string query_string;
    std::string version;

    std::string body;
};

struct t_client
{
    int listen_socket;

    int fd;
    std::string req_buffer;
    std::string res_buffer;
    time_t timing;

    int fd_in;
    int fd_out;
    std::string cgi_output;
    pid_t pid;
    bool is_cgi;
    time_t cgi_timing;

    HttpRequest req;
    bool is_header_parsed;
    size_t content_len;
    bool is_chunked;

    t_client();
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

    bool has_explicit_root;
    bool has_explicit_index;

    std::string upload_store;

    size_t      client_max_body_size;
    bool        has_client_max_body_size;

    int         redirect_code;
    std::string redirect;

    std::map<std::string, std::string> cgi_pass;
    
    bool        internal;

    // allow_upload — is POST upload permitted at this location?
    // Set by "allow_upload on;" in location block.
    // Ilyas checks this in handlePOST before saving files.
    bool        allow_upload;

    // upload_path — where to save uploaded files.
    // Set by "upload_path www/upload;" in location block.
    // Used by Ilyas's handlePOST as baseDir.
    std::string upload_path;

    // allow_delete — is DELETE permitted at this location?
    // Set by "allow_delete on;" in location block.
    // Ilyas checks this in handleDELETE before removing files.
    bool        allow_delete;

    LocationConfig();
};

class ServerConfig
{
public:
    std::string host;

    // int port; we need list of ports not just one
    std::vector<int> ports;

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
    std::string getBody() const;
    bool        startCGI(int &fd_in, int &fd_out, pid_t &pid);
    void        releaseBody(std::string &body);
    static std::string buildResponseHeader(
        const std::string &cgi_headers,
        size_t body_size);
    static std::string wrapResponse(const std::string &cgi_output);//to static

private:

    HttpRequest _request;
    std::string _script_path;
    std::string _cgi_executable;

    char      **buildEnv()                                  const;
    void        freeEnv(char **env)                         const;
    std::string unchunkBody(const std::string &chunked)     const;
};

// int     init_socket(const ServerConfig& serv);
// void    multiplexing(int sfd, const ServerConfig& serv);
std::vector<int> init_socket(const std::vector<ServerConfig>& servers,
							std::map<int, std::vector<ServerConfig> >& socket_server);
void	multiplexing(std::vector<int> &sockets,
					std::map<int, std::vector<ServerConfig> >& socket_server);

std::string buildResponse(int code,
                          const std::string& body,
                          const std::string& mime,
                          const std::vector<std::string>& extraHeaders);
std::string buildPath(const std::string& requestPath,
                      const LocationConfig& loc);
std::string buildPath(const std::string& requestPath,
                      const LocationConfig& loc,
                      bool appendIndex);

bool        isFile(const std::string& path);
bool        isDirectory(const std::string& path);
bool        isSymlink(const std::string& path);
std::string readFile(const std::string& path);
std::string getMimeType(const std::string& path);
std::string normalizePath(const std::string& path);
std::string statusMessage(int code);
std::string redirectResponse(int code, const std::string& url);
bool        isMethodAllowed(const LocationConfig& loc, const std::string& method);
std::string buildAllowHeader(const LocationConfig& loc);
std::string buildResponse(int code,
                          const std::string& body,
                          const std::string& mime);
 bool isInsideRoot(const std::string& path, const std::string& root);
bool fileExists(const std::string& path);

 std::string handleMultipart(
    const HttpRequest& req,
    const ServerConfig& serv,
    const std::string& baseDir);
 std::string git (const std::string& body);
 std::string getHeaderValue(const std::string& body,
                                  const std::string& header);
std::string buildResponse(int code,
                          const std::string& body,
                          const std::string& mime,
                          const std::vector<std::string>& extraHeaders);
std::string errorResponse(int code,
                          const std::string& allowHeader,
                          const ServerConfig* config);
std::vector<std::string> readDirectory(const std::string& path);
std::string generateAutoIndex(const std::string& requestPath,
                              const std::string& directoryPath,
                              const std::vector<std::string>& files);

std::string dispatchRequest(const HttpRequest& req, const ServerConfig& Serv);
std::string handleRequest(const HttpRequest& req,
                          const LocationConfig& loc,
                          const ServerConfig* config);
std::string handlePOST(const HttpRequest& req,
                       const ServerConfig& serv);

#endif
