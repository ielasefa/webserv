/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   file_serving.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 21:40:26 by iel-asef          #+#    #+#             */
/*   Updated: 2026/07/02 21:54:10 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

bool isSymlink(const std::string& path)
{
    struct stat s;
    if (lstat(path.c_str(), &s) != 0)
        return false;
    return S_ISLNK(s.st_mode);
}

std::string redirect301(const std::string& to)
{
    std::vector<std::string> headers;
    headers.push_back("Location: " + to);
    return buildResponse(301, "", "text/plain", headers);
}

std::string serveFile(const std::string& path, const ServerConfig* config)
{
    if (isSymlink(path))
        return errorResponse(403, "", config);

    std::string content = readFile(path);
    return buildResponse(200, content, getMimeType(path));
}
std::string handleRequest(const HttpRequest& req,
                          const LocationConfig& loc,
                          const ServerConfig* config)
{
    std::string cleanPath = normalizePath(req.path);

    if (cleanPath.find("..") != std::string::npos)
        return errorResponse(403, "", config);

    std::string fullPath = buildPath(cleanPath, loc);

    bool hasSlash =
        !req.path.empty() &&
        req.path[req.path.size() - 1] == '/';

    if (isSymlink(fullPath))
        return errorResponse(403, "", config);

    if (isFile(fullPath))
        return serveFile(fullPath, config);

    if (isDirectory(fullPath))
    {
        if (!hasSlash && cleanPath != "/")
            return redirect301(cleanPath + "/");

        std::string indexPath = buildPath(cleanPath, loc, true);

        if (isFile(indexPath))
            return serveFile(indexPath, config);

        if (!loc.autoindex)
            return errorResponse(403, "", config);

        std::vector<std::string> files = readDirectory(fullPath);
        return generateAutoIndex(cleanPath, fullPath, files);
    }

    return errorResponse(404, "", config);
}