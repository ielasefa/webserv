#include "Webserv.hpp"

bool fileExists(const std::string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

#include <fstream>
#include <sstream>

std::string readFile(const std::string& path)
{
    std::ifstream file(path.c_str());

    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}