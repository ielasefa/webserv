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

std::string handlePOST(const Request& req, const Location& loc)
{
    if (!loc.allow_post)
        return errorResponse(403);

    if (req.path.empty())
        return errorResponse(400);

    if (req.path.find("..") != std::string::npos)
        return errorResponse(403);

    std::map<std::string, std::string>::const_iterator it =
        req.headers.find("Content-Length");

    if (it == req.headers.end())
        return errorResponse(411);

    size_t contentLength = std::atoi(it->second.c_str());

    if (req.body.size() != contentLength)
        return errorResponse(400);

    std::string basePath = buildPath(req.path, loc);

    std::map<std::string, std::string>::const_iterator ct =
        req.headers.find("Content-Type");

    if (ct != req.headers.end() &&
        ct->second.find("multipart/form-data") != std::string::npos)
    {
        std::string filename = extractFilename(req.body);
        std::string content  = extractContent(req.body);

        if (filename.empty() || content.empty())
            return errorResponse(400);

        if (filename.find("/") != std::string::npos ||
            filename.find("..") != std::string::npos)
            return errorResponse(403);

        std::string path = basePath + "/" + filename;

        std::ofstream file(path.c_str(), std::ios::binary);
        if (!file.is_open())
            return errorResponse(500);

        file.write(content.c_str(), content.size());
        file.close();

        return buildResponse(201, "", "");
    }

    std::ofstream file(basePath.c_str(), std::ios::binary);
    if (!file.is_open())
        return errorResponse(500);

    file.write(req.body.c_str(), contentLength);
    file.close();

    return buildResponse(201, "", "");
}
std::string statusMessage(int code);
std::string dispatchRequest(const Request& req);
