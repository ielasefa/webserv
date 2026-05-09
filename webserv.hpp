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

#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "config/ConfigParser.hpp"
#include "cgi/CGIHandler.hpp"


#endif
