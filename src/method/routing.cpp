/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   routing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 19:14:11 by iel-asef          #+#    #+#             */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

std::string buildPath(const std::string& requestPath,
                      const LocationConfig& loc,
                      bool appendIndex);

static std::string stripQueryString(const std::string& path)
{
    size_t pos = path.find('?');

    if (pos == std::string::npos)
        return path;
    return path.substr(0, pos);
}

static std::string canonicalLocationPath(const std::string& path)
{
    if (path.size() > 1 && path[path.size() - 1] == '/')
        return path.substr(0, path.size() - 1);
    return path;
}

static bool locationMatches(const std::string& requestPath,
                            const std::string& locationPath)
{
    if (locationPath.empty())
        return false;
    if (locationPath == "/")
        return true;
    if (requestPath == locationPath)
        return true;
    if (requestPath.size() <= locationPath.size())
        return false;
    if (requestPath.compare(0, locationPath.size(), locationPath) != 0)
        return false;
    if (locationPath[locationPath.size() - 1] == '/')
        return true;
    return requestPath[locationPath.size()] == '/';
}

LocationConfig ServerConfig::matchLocation(const std::string& path) const
{
    if (locations.empty())
        return LocationConfig();

    std::string requestPath = normalizePath(path);
    LocationConfig best;
    bool found = false;

    for (size_t i = 0; i < locations.size(); ++i)
    {
        const LocationConfig& loc = locations[i];

        if (!locationMatches(requestPath, loc.path))
            continue;
        if (!found || loc.path.size() > best.path.size())
        {
            best = loc;
            found = true;
        }
    }

    if (!found)
        return LocationConfig();

    if (best.path != "/" && best.cgi_pass.empty())
    {
        for (size_t i = 0; i < locations.size(); ++i)
        {
            if (locations[i].path == "/")
            {
                best.cgi_pass = locations[i].cgi_pass;
                break;
            }
        }
    }
    return best;
}

std::string redirectResponse(int code, const std::string& url)
{
    std::vector<std::string> headers;

    headers.push_back("Location: " + url);
    return buildResponse(code, "", "text/plain", headers);
}

std::string normalizeSlashes(const std::string& path)
{
    std::string result;

    for (size_t i = 0; i < path.size(); ++i)
    {
        if (path[i] == '/' && !result.empty()
            && result[result.size() - 1] == '/')
            continue;
        result += path[i];
    }
    if (result.empty())
        return "/";
    return result;
}

std::string normalizePath(const std::string& path)
{
    std::string cleanPath = stripQueryString(path);
    std::vector<std::string> parts;
    std::stringstream stream(cleanPath);
    std::string part;

    while (std::getline(stream, part, '/'))
    {
        if (part.empty() || part == ".")
            continue;
        if (part == "..")
        {
            if (!parts.empty())
                parts.pop_back();
            continue;
        }
        parts.push_back(part);
    }

    std::string result = "/";

    for (size_t i = 0; i < parts.size(); ++i)
    {
        result += parts[i];
        if (i + 1 < parts.size())
            result += "/";
    }

    if (cleanPath.size() > 1 && cleanPath[cleanPath.size() - 1] == '/'
        && result[result.size() - 1] != '/')
        result += "/";

    return result;
}

std::string buildPath(const std::string& requestPath,
                      const LocationConfig& loc)
{
    return buildPath(requestPath, loc, false);
}

std::string buildPath(const std::string& requestPath, const LocationConfig& loc, bool appendIndex)
{
    std::string root = loc.root;
    std::string rel = normalizePath(requestPath);
    std::string locationPath = canonicalLocationPath(loc.path);
    std::string strippedRel = rel;
    std::string unstrippedRel = rel;

    if (loc.has_explicit_root && locationPath != "/"
        && locationMatches(rel, loc.path))
        strippedRel = rel.substr(locationPath.size());

    while (!strippedRel.empty() && strippedRel[0] == '/')
        strippedRel.erase(0, 1);
    while (!unstrippedRel.empty() && unstrippedRel[0] == '/')
        unstrippedRel.erase(0, 1);
    while (root.size() > 1 && root[root.size() - 1] == '/')
        root.erase(root.size() - 1);

    std::string strippedPath = root;

    if (!strippedRel.empty())
    {
        if (!strippedPath.empty())
            strippedPath += "/";
        strippedPath += strippedRel;
    }
    if (strippedRel.empty() && !strippedPath.empty()
        && strippedPath[strippedPath.size() - 1] != '/')
        strippedPath += "/";

    std::string finalPath = strippedPath;

    if (loc.has_explicit_root && locationPath != "/"
        && locationMatches(rel, loc.path))
    {
        std::string rawPath = root;

        if (!unstrippedRel.empty())
        {
            if (!rawPath.empty())
                rawPath += "/";
            rawPath += unstrippedRel;
        }
        if (unstrippedRel.empty() && !rawPath.empty()
            && rawPath[rawPath.size() - 1] != '/')
            rawPath += "/";

        if (fileExists(rawPath) || isDirectory(rawPath))
            finalPath = rawPath;
        else if (fileExists(strippedPath) || isDirectory(strippedPath))
            finalPath = strippedPath;
    }

    if (appendIndex)
    {
        if (!finalPath.empty() && finalPath[finalPath.size() - 1] != '/')
            finalPath += "/";
        if (!loc.index.empty())
            finalPath += loc.index;
        else
            finalPath += "index.html";
    }
    return finalPath;
}
