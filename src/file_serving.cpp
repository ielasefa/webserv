/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   file_serving.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 19:50:53 by iel-asef          #+#    #+#             */
/*   Updated: 2026/04/15 11:41:37 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

static std::string getMimeType(const std::string& path)
{
    if (path.find(".html") != std::string::npos)
        return "text/html";
    if (path.find(".css") != std::string::npos)
        return "text/css";
    if (path.find(".js") != std::string::npos)
        return "application/javascript";
    if (path.find(".png") != std::string::npos)
        return "image/png";
    if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos)
        return "image/jpeg";
    if (path.find(".txt") != std::string::npos)
        return "text/plain";

    return "text/plain";
}

static std::string buildResponse(int code,
                                 const std::string& body,
                                 const std::string& mime)
{
    std::string res;

    res += "HTTP/1.1 " + statusMessage(code) + "\r\n";

    if (!mime.empty())
        res += "Content-Type: " + mime + "\r\n";

    std::ostringstream ss;
    ss << body.size();

    res += "Content-Length: " + ss.str() + "\r\n";
    res += "Connection: close\r\n\r\n";

    res += body;

    return res;
}

std::string serveFile(const std::string& path)
{
    if (!isFile(path))
        return errorResponse(404);

    std::string content = readFile(path);

    return buildResponse(200, content, getMimeType(path));
}

std::string redirect301(const std::string& newPath)
{
    std::string res;

    res += "HTTP/1.1 301 Moved Permanently\r\n";
    res += "Location: " + newPath + "\r\n";
    res += "Content-Length: 0\r\n";
    res += "Connection: close\r\n\r\n";

    return res;
}

std::string handleRequest(const std::string& requestPath)
{
    if (requestPath.find("..") != std::string::npos)
        return errorResponse(403);

    std::string cleanPath = normalizePath(requestPath);

    Location loc = matchLocation(cleanPath);

    std::string path = buildPath(cleanPath, loc);

    if (isFile(path))
        return serveFile(path);

    if (isDirectory(path))
    {
        if (!cleanPath.empty() && cleanPath[cleanPath.size() - 1] != '/')
            return redirect301(cleanPath + "/");

        std::string index = path;
        if (index[index.size() - 1] != '/')
            index += "/";
        index += "index.html";

        if (isFile(index))
            return serveFile(index);

        if (!loc.autoindex)
            return errorResponse(403);

        std::vector<std::string> files = readDirectory(path);

        return generateAutoIndex(cleanPath, files);
    }

    return errorResponse(404);
}