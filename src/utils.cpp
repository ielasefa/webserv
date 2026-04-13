#include "webserv.hpp"

bool isFile(const std::string& path)
{
    struct stat s;
    return (stat(path.c_str(), &s) == 0 && S_ISREG(s.st_mode));
}

bool isDirectory(const std::string& path)
{
    struct stat s;
    return (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode));
}

std::string readFile(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string sizeToString(size_t value)
{
    std::stringstream stream;
    stream << value;
    return stream.str();
}