#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <sstream>

struct Location
{
    std::string path;
    std::string root;
    bool autoindex;
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
std::string handleRequest(const std::string& requestPath);
std::string errorResponse(int code);

#endif