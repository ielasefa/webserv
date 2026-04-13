#include "webserv.hpp"

std::string serveFile(const std::string& path)
{
    std::string content = readFile(path);

    if (content.empty())
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";

    std::string res;
    res += "HTTP/1.1 200 OK\r\n";
    res += "Content-Length: " + sizeToString(content.size()) + "\r\n";
    res += "Content-Type: text/html\r\n";
    res += "Connection: close\r\n\r\n";
    res += content;

    return res;
}

std::string handleRequest(const std::string& requestPath)
{
    if (requestPath.find("..") != std::string::npos)
        return "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";

    Location loc = matchLocation(requestPath);
    std::string path = buildPath(requestPath, loc);

    if (isFile(path))
        return serveFile(path);

    if (isDirectory(path))
    {
        std::string index = path + "/index.html";

        if (isFile(index))
            return serveFile(index);

        if (!loc.autoindex)
            return "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";

        std::vector<std::string> files = readDirectory(path);

        return generateAutoIndex(requestPath, files);
    }

    return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
}