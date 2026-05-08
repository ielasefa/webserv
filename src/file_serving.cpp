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

std::string handleRequest(const Request& request, const Location& loc)
{
    if (request.path.find("..") != std::string::npos)
        return errorResponse(403);

    std::string cleanPath = normalizePath(request.path);

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
        if (!loc.index.empty())
            index += loc.index;
        else
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