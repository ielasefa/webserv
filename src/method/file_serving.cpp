/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   file_serving.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 21:40:26 by iel-asef          #+#    #+#             */
/*   Updated: 2026/08/24 23:54:00 by iel-asef         ###   ########.fr       */
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

static bool hasReadPermission(const struct stat& s)
{
    return (s.st_mode & (S_IRUSR | S_IRGRP | S_IROTH)) != 0;
}

static bool hasExecutePermission(const struct stat& s)
{
    return (s.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0;
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

    struct stat fileStat;

    if (stat(path.c_str(), &fileStat) != 0)
        return errorResponse(404, "", config);

    if (!S_ISREG(fileStat.st_mode))
        return errorResponse(404, "", config);

    if (!hasReadPermission(fileStat))
        return errorResponse(403, "", config);

    std::string content = readFile(path);

    return buildResponse(200, content, getMimeType(path));
}

std::string handleRequest(const HttpRequest& req,
                          const LocationConfig& loc,
                          const ServerConfig* config)
{
    std::string cleanPath = normalizePath(req.path);

    /*
     * Path traversal protection.
     */
    if (cleanPath.find("..") != std::string::npos)
        return errorResponse(403, "", config);

    bool hasSlash =
        !req.path.empty() &&
        req.path[req.path.size() - 1] == '/';

    std::string fullPath;

    /*
     * Upload location.
     */
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
std::cerr
    << "REQ PATH=[" << req.path << "] "
    << "CLEAN PATH=[" << cleanPath << "] "
    << "LOC PATH=[" << loc.path << "] "
    << "LOC ROOT=[" << loc.root << "] "
    << "FULL PATH=[" << fullPath << "]"
    << std::endl;
    struct stat pathStat;

    /*
     * Symlink => forbidden.
     */
    if (lstat(fullPath.c_str(), &pathStat) == 0 &&
        S_ISLNK(pathStat.st_mode))
    {
        return errorResponse(403, "", config);
    }

    /*
     * Path does not exist.
     */
    if (stat(fullPath.c_str(), &pathStat) != 0)
        return errorResponse(404, "", config);

    /*
     * Regular file.
     */
    if (S_ISREG(pathStat.st_mode))
    {
        if (!hasReadPermission(pathStat))
            return errorResponse(403, "", config);

        return serveFile(fullPath, config);
    }

    /*
     * Directory.
     */
    if (S_ISDIR(pathStat.st_mode))
    {
        /*
         * Directory must be readable and searchable.
         *
         * chmod 000 directory
         * => 403 Forbidden
         */
        if (!hasReadPermission(pathStat) ||
            !hasExecutePermission(pathStat))
        {
            return errorResponse(403, "", config);
        }

        /*
         * Directory URL must end with "/".
         */
        if (!hasSlash && cleanPath != "/")
            return redirect301(cleanPath + "/");

        /*
         * Upload directory handling.
         */
        if (loc.allow_upload && !loc.upload_path.empty())
        {
            if (!loc.autoindex)
                return errorResponse(403, "", config);

            std::vector<std::string> files =
                readDirectory(fullPath);

            return generateAutoIndex(
                cleanPath,
                fullPath,
                files
            );
        }

        /*
         * Search for configured index file.
         */
        std::string indexPath =
            buildPath(cleanPath, loc, true);

        if (isFile(indexPath))
        {
            struct stat indexStat;

            if (stat(indexPath.c_str(), &indexStat) != 0)
                return errorResponse(404, "", config);

            if (!hasReadPermission(indexStat))
                return errorResponse(403, "", config);

            return serveFile(indexPath, config);
        }

        /*
         * Directory exists but no index
         * and autoindex is disabled.
         */
        if (!loc.autoindex)
            return errorResponse(403, "", config);

        /*
         * Autoindex enabled.
         */
        std::vector<std::string> files =
            readDirectory(fullPath);

        return generateAutoIndex(
            cleanPath,
            fullPath,
            files
        );
    }

    /*
     * Unsupported filesystem object.
     */
    return errorResponse(404, "", config);
}