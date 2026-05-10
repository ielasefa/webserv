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

    if (access(path.c_str(), R_OK) != 0)
        return errorResponse(403);

    std::string content = readFile(path);

    if (!content.empty())
        return buildResponse(200, content, getMimeType(path));

    struct stat s;
    if (stat(path.c_str(), &s) != 0)
        return errorResponse(404);

    if (s.st_size > 0)
        return errorResponse(500);

    return buildResponse(200, content, getMimeType(path));
}

std::string redirect301(const std::string& newPath)
{
    std::vector<std::string> headers;
    headers.push_back("Location: " + newPath);

    return buildResponse(301, "", "", headers);
}

std::string handleRequest(const Request& request, const Location& loc)
{
    if (request.path.find("..") != std::string::npos)
        return errorResponse(403);

    std::string cleanPath = normalizePath(request.path);
    bool hasTrailingSlash = !request.path.empty() && request.path[request.path.size() - 1] == '/';

    if (cleanPath.find("..") != std::string::npos)
        return errorResponse(403);

    std::string path = buildPath(cleanPath, loc);

    if (isFile(path))
        return serveFile(path);

    if (isDirectory(path))
    {
        if (!hasTrailingSlash && cleanPath != "/")
            return redirect301(cleanPath + "/");

        std::string index = buildPath(cleanPath, loc, true);

        if (isFile(index))
            return serveFile(index);

        if (!loc.autoindex)
            return errorResponse(403);

        std::vector<std::string> files = readDirectory(path);

        return generateAutoIndex(cleanPath, files);
    }

    return errorResponse(404);
}