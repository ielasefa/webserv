/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   file_serving.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 21:40:26 by iel-asef          #+#    #+#             */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

static bool componentIsSymlink(const std::string& path)
{
    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_NOFOLLOW);

    if (fd >= 0)
    {
        close(fd);
        return false;
    }
    return errno == ELOOP;
}

bool isSymlink(const std::string& path)
{
    if (path.empty())
        return false;

    std::string current;
    size_t pos = 0;

    if (path[0] == '/')
    {
        current = "/";
        pos = 1;
    }

    while (pos < path.size())
    {
        while (pos < path.size() && path[pos] == '/')
            ++pos;
        if (pos == path.size())
            break;

        size_t end = path.find('/', pos);
        std::string part;

        if (end == std::string::npos)
            part = path.substr(pos);
        else
            part = path.substr(pos, end - pos);

        if (part != ".")
        {
            if (!current.empty() && current[current.size() - 1] != '/')
                current += "/";
            current += part;
            if (componentIsSymlink(current))
                return true;
        }

        if (end == std::string::npos)
            break;
        pos = end + 1;
    }
    return false;
}

static bool hasReadPermission(const struct stat& info)
{
    return (info.st_mode & (S_IRUSR | S_IRGRP | S_IROTH)) != 0;
}

static bool hasExecutePermission(const struct stat& info)
{
    return (info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0;
}

static std::string pathErrorResponse(const ServerConfig* config)
{
    if (errno == EACCES || errno == EPERM)
        return errorResponse(403, "", config);
    return errorResponse(404, "", config);
}

static bool hasLocationSpecificIndex(const LocationConfig& loc,
                                     const ServerConfig* config)
{
    if (loc.index.empty())
        return false;
    if (config == NULL || config->index.empty())
        return true;
    return loc.index != config->index;
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
        return pathErrorResponse(config);
    if (!S_ISREG(fileStat.st_mode))
        return errorResponse(404, "", config);
    if (!hasReadPermission(fileStat))
        return errorResponse(403, "", config);

    return buildResponse(200, readFile(path), getMimeType(path));
}

static std::string buildUploadPath(const std::string& cleanPath, const LocationConfig& loc)
{
    std::string relative = cleanPath;

    if (loc.path != "/"
        && relative.compare(0, loc.path.size(), loc.path) == 0)
        relative = relative.substr(loc.path.size());

    while (!relative.empty() && relative[0] == '/')
        relative.erase(0, 1);

    std::string result = loc.upload_path;

    if (!result.empty() && result[result.size() - 1] != '/')
        result += "/";
    result += relative;
    return result;
}

std::string handleRequest(const HttpRequest& req, const LocationConfig& loc, const ServerConfig* config)
{
    std::string cleanPath = normalizePath(req.path);

    if (cleanPath.empty())
        return errorResponse(400, "", config);

    bool hasSlash = cleanPath[cleanPath.size() - 1] == '/';
    std::string fullPath;

    if (loc.allow_upload && !loc.upload_path.empty())
        fullPath = buildUploadPath(cleanPath, loc);
    else
        fullPath = buildPath(cleanPath, loc);

    if (isSymlink(fullPath))
        return errorResponse(403, "", config);

    struct stat pathStat;

    if (stat(fullPath.c_str(), &pathStat) != 0)
        return pathErrorResponse(config);

    if (S_ISREG(pathStat.st_mode))
    {
        if (!hasReadPermission(pathStat))
            return errorResponse(403, "", config);
        return serveFile(fullPath, config);
    }

    if (!S_ISDIR(pathStat.st_mode))
        return errorResponse(404, "", config);

    if (!hasExecutePermission(pathStat))
        return errorResponse(403, "", config);

    if (!hasSlash && cleanPath != "/")
        return redirect301(cleanPath + "/");

    if (loc.allow_upload && !loc.upload_path.empty())
    {
        if (!loc.autoindex || !hasReadPermission(pathStat))
            return errorResponse(403, "", config);

        std::vector<std::string> files = readDirectory(fullPath);
        return generateAutoIndex(cleanPath, fullPath, files);
    }

    std::string indexPath = buildPath(cleanPath, loc, true);

    if (isSymlink(indexPath))
        return errorResponse(403, "", config);

    struct stat indexStat;

    if (stat(indexPath.c_str(), &indexStat) == 0)
    {
        if (S_ISREG(indexStat.st_mode))
        {
            if (!hasReadPermission(indexStat))
                return errorResponse(403, "", config);
            return serveFile(indexPath, config);
        }
    }
    else if (errno == EACCES || errno == EPERM)
        return errorResponse(403, "", config);

    if (hasLocationSpecificIndex(loc, config))
        return errorResponse(404, "", config);

    if (!loc.autoindex || !hasReadPermission(pathStat))
        return errorResponse(403, "", config);

    std::vector<std::string> files = readDirectory(fullPath);
    return generateAutoIndex(cleanPath, fullPath, files);
}
