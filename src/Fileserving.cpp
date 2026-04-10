#include <iostream>
#include <string>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <map>

bool fileExists(const std::string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
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

std::string buildPath(const std::string& requestPath)
{
    std::string root = "www";

    if (requestPath == "/")
        return root + "/index.html";

    return root + requestPath;
}

static std::string getExtension(const std::string& path)
{
    size_t dot = path.rfind('.');
    if (dot == std::string::npos)
        return "";
    return path.substr(dot);
}

std::string getMimeType(const std::string& path)
{
    static std::map<std::string, std::string> mimeTypes;

    if (mimeTypes.empty())
    {
        mimeTypes[".html"] = "text/html";
        mimeTypes[".css"]  = "text/css";
        mimeTypes[".js"]   = "application/javascript";
        mimeTypes[".png"]  = "image/png";
        mimeTypes[".jpg"]  = "image/jpeg";
        mimeTypes[".jpeg"] = "image/jpeg";
        mimeTypes[".gif"]  = "image/gif";
        mimeTypes[".txt"]  = "text/plain";
    }

    std::string ext = getExtension(path);

    if (mimeTypes.find(ext) != mimeTypes.end())
        return mimeTypes[ext];

    return "application/octet-stream";
}

std::string serveFile(const std::string& requestPath)
{
    if (requestPath.find("..") != std::string::npos)
    {
        return "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    }

    std::string path = buildPath(requestPath);

    if (!fileExists(path))
    {
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    }

    std::string content = readFile(path);
    std::string mime = getMimeType(path);

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
    response += "Content-Type: " + mime + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += content;

    return response;
}

int main()
{
    std::string response = serveFile("/index.html");
    std::cout << response << std::endl;
}