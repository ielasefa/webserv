#include "webserv.hpp"
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>

std::string extractFilename(const std::string& body)
{
    size_t pos = body.find("filename=\"");
    if (pos == std::string::npos)
        return "";

    pos += 10;
    size_t end = body.find("\"", pos);

    return body.substr(pos, end - pos);
}

std::string extractContent(const std::string& body)
{
    size_t start = body.find("\r\n\r\n");
    if (start == std::string::npos)
        return "";

    start += 4;

    size_t end = body.rfind("--");
    if (end == std::string::npos)
        return body.substr(start);

    return body.substr(start, end - start);
}

std::string handlePOST(const Request& req)
{
    if (req.path.empty())
        return errorResponse(400);

    if (req.path.find("..") != std::string::npos)
        return errorResponse(403);

    if (req.path.find("/upload") != 0)
        return errorResponse(403);

    if (req.headers.find("Content-Length") == req.headers.end())
        return errorResponse(411);

    size_t contentLength = std::atoi(req.headers.at("Content-Length").c_str());

    if (req.body.size() < contentLength)
        return errorResponse(400);

    if (req.headers.find("Content-Type") != req.headers.end() &&
        req.headers.at("Content-Type").find("multipart/form-data") != std::string::npos)
    {
        std::string filename = extractFilename(req.body);
        std::string content = extractContent(req.body);

        if (filename.empty() || content.empty())
            return errorResponse(400);

        if (filename.find("/") != std::string::npos || filename.find("..") != std::string::npos)
            return errorResponse(403);

        std::string path = "www/upload/" + filename;

        std::ofstream file(path.c_str(), std::ios::binary);
        if (!file.is_open())
            return errorResponse(500);

        file.write(content.c_str(), content.size());
        file.close();

        std::string res;
        res += "HTTP/1.1 201 Created\r\n";
        res += "Content-Length: 0\r\n";
        res += "Connection: close\r\n\r\n";

        return res;
    }

    std::string path = "www" + req.path;

    std::ofstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return errorResponse(500);

    file.write(req.body.c_str(), contentLength);
    file.close();

    std::string res;
    res += "HTTP/1.1 201 Created\r\n";
    res += "Content-Length: 0\r\n";
    res += "Connection: close\r\n\r\n";

    return res;
}

std::string statusMessage(int code);
std::string dispatchRequest(const Request& req);
