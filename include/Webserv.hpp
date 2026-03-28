#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <string>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

//fileserving.cpp
bool fileExists(const std::string& path);
std::string readFile(const std::string& path);
std::string buildPath(const std::string& requestPath);
std::string serveFile(const std::string& requestPath);



#endif