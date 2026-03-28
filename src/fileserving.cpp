#include <iostream>
#include <string>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

bool fileExists(const std::string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string readFile(const std::string& path)
{
    std::ifstream file(path.c_str());

    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

std::string buildPath(const std::string& requestPath)
{
    std::string root = "www";
    if (requestPath == "/")
        return root + "/index.html";
    return root + requestPath;
}

std::string serveFile(const std::string& requestPath)
{
    std::string path = buildPath(requestPath);

    if (!fileExists(path))
    {
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }

    std::string content = readFile(path);

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
    response += "\r\n";
    response += content;

    return response;
}

int main()
{
    std::string response = serveFile("/index.html");
    std::cout << response << std::endl;
}