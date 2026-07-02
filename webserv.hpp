#ifndef WEBSERV_HPP
#define WEBSERV_HPP


//---------------------- jaafar ----------------------

#include "src/parsing/webserv.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <csignal>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern const int    BACKLOG;
extern const int    POLL_TIMEOUT_MS;
extern const int    CGI_TIMEOUT_S;
extern const char  *DEFAULT_CONFIG_PATH;
extern const int    READ_BUFFER_SIZE;

//---------------------------------------------------

//---------------------- ilyas ----------------------

std::string dispatchRequest(const HttpRequest& req , const ServerConfig& serv);

LocationConfig matchLocation(const std::string& requestPath);
std::string buildPath(const std::string& requestPath, const LocationConfig& loc);
std::string buildPath(const std::string& requestPath, const LocationConfig& loc, bool appendIndex);

bool isFile(const std::string& path);
bool isDirectory(const std::string& path);
std::string readFile(const std::string& path);
std::string sizeToString(size_t value);
std::string generateAutoIndex(const std::string& requestPath,
                              const std::string& directoryPath,
                              const std::vector<std::string>& files);
std::string serveFile(const std::string& path, const ServerConfig* config = NULL);
std::vector<std::string> readDirectory(const std::string& path);
std::string generateAutoIndex(const std::string& requestPath,
                              const std::vector<std::string>& files);
std::string handleRequest(const HttpRequest& request , const LocationConfig& loc, const ServerConfig* config = NULL);
std::string errorResponse(int code, const std::string& allowHeader = "", const ServerConfig* config = NULL);
std::string readBody(int fd, size_t contentLength);
std::string normalizePath(const std::string& path);
std::string handlePOST(const HttpRequest& request , const ServerConfig& serv);
std::string handleDELETE(const HttpRequest& request ,const LocationConfig& loc, const ServerConfig* config = NULL);
bool fileExists(const std::string& path);
std::string buildResponse(int code,
                          const std::string& body,
                          const std::string& mime,
                          const std::vector<std::string>& extraHeaders = std::vector<std::string>());
std::string getMimeType(const std::string& path);
std::string redirect301(const std::string& newPath);
std::string statusMessage(int code);
bool isMethodAllowed(const LocationConfig& loc, const std::string& method);
std::string buildAllowHeader(const LocationConfig& loc);
std::string redirectResponse(int code, const std::string& url);
bool isSymlink(const std::string& path);

//---------------------------------------------------

//---------------------- ayoub ----------------------

#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <string.h>
#include <netinet/in.h>
#include <map>

typedef struct s_client {
	int fd;
	std::string req_buffer;
	std::string res_buffer;
}	t_client;

typedef struct s_header
{
	std::string first_line;
	std::string method;
	std::string path;
	std::string version;
	std::string other;
}	t_header;

int	init_socket();
void	add_epoll(int epfd, int fd);
void	multiplexing(int sfd ,const ServerConfig& serv);
HttpRequest	parsing_header(std::string req_buffer);

//---------------------------------------------------

#endif

