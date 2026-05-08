#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <csignal>


struct Location
{
    std::string path;        
    std::string root;             

    std::string index;             
    bool autoindex;                

    std::vector<std::string> allowed_methods;

    bool allow_upload;             
    std::string upload_path;       
    
    bool allow_post;               

    bool allow_delete;             
    size_t client_max_body_size;  
    std::map<int, std::string> error_pages; 
};

struct Request
{
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
};


extern std::vector<Location> locations;

void initLocations();
Location matchLocation(const std::string& requestPath);
std::string buildPath(const std::string& requestPath, const Location& loc);

bool isFile(const std::string& path);
bool isDirectory(const std::string& path);
std::string readFile(const std::string& path);
std::string sizeToString(size_t value);

std::string serveFile(const std::string& path);
std::vector<std::string> readDirectory(const std::string& path);
std::string generateAutoIndex(const std::string& requestPath,
                              const std::vector<std::string>& files);
std::string handleRequest(const Request& request , const Location& loc);
std::string errorResponse(int code);
std::string readBody(int fd, size_t contentLength);
std::string normalizePath(const std::string& path);
std::string handlePOST(const Request& request , const Location& loc);
std::string handleDELETE(const Request& request ,const Location& loc);
bool fileExists(const std::string& path);
static std::string buildResponse(int code,
                                 const std::string& body,
                                 const std::string& mime);
std::string getMimeType(const std::string& path);
std::string redirect301(const std::string& newPath);
std::string statusMessage(int code);
bool isMethodAllowed(const Location& loc, const std::string& method);



#endif