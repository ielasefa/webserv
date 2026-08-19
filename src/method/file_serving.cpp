/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   file_serving.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 21:40:26 by iel-asef          #+#    #+#             */
/*   Updated: 2026/07/27 14:10:00 by iel-asef         ###   ########.fr       */
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

std::string serveFile(const std::string& path,
                      const ServerConfig* config)
{
    if (isSymlink(path))
        return errorResponse(403, "", config);

    if (access(path.c_str(), R_OK) != 0)
        return errorResponse(403, "", config);

    std::string content = readFile(path);

    if (content.empty() && fileExists(path))
        return errorResponse(403, "", config);

    return buildResponse(200, content, getMimeType(path));
}

std::string handleRequest(const HttpRequest& req,
                          const LocationConfig& loc,
                          const ServerConfig* config)
{
    std::string cleanPath = normalizePath(req.path);

    if (cleanPath.find("..") != std::string::npos)
        return errorResponse(403, "", config);

    bool hasSlash =
        !req.path.empty() &&
        req.path[req.path.size() - 1] == '/';

    std::string fullPath;

    if (loc.allow_upload && !loc.upload_path.empty())
    {
        std::string relative = cleanPath;

        if (loc.path != "/" &&
            relative.compare(0, loc.path.length(), loc.path) == 0)
        {
            relative = relative.substr(loc.path.length());
        }

        while (!relative.empty() && relative[0] == '/')
            relative.erase(0, 1);

        fullPath = loc.upload_path;

        if (!fullPath.empty() &&
            fullPath[fullPath.size() - 1] != '/')
        {
            fullPath += '/';
        }

        fullPath += relative;
    }
    else
    {
        fullPath = buildPath(cleanPath, loc);
    }

    struct stat pathStat;

    if (lstat(fullPath.c_str(), &pathStat) == 0 && S_ISLNK(pathStat.st_mode))
        return errorResponse(403, "", config);

    if (stat(fullPath.c_str(), &pathStat) != 0)
    {
        if (errno == EACCES || errno == EPERM)
            return errorResponse(403, "", config);
        return errorResponse(404, "", config);
    }

    if (S_ISREG(pathStat.st_mode))
        return serveFile(fullPath, config);

    if (S_ISDIR(pathStat.st_mode))
    {
        if (access(fullPath.c_str(), X_OK) != 0)
            return errorResponse(403, "", config);

        if (!hasSlash && cleanPath != "/")
            return redirect301(cleanPath + "/");

        if (loc.allow_upload && !loc.upload_path.empty())
        {
            if (!loc.autoindex)
                return errorResponse(403, "", config);

            std::vector<std::string> files =
                readDirectory(fullPath);

            return generateAutoIndex(cleanPath, fullPath, files);
        }

        std::string indexPath =
            buildPath(cleanPath, loc, true);

        if (isFile(indexPath))
            return serveFile(indexPath, config);

        if (!loc.autoindex)
            return errorResponse(403, "", config);

        std::vector<std::string> files = readDirectory(fullPath);

        return generateAutoIndex(cleanPath,
                                 fullPath,
                                 files);
    }

    return errorResponse(404, "", config);
}