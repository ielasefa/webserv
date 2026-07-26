/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   routing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaloulid <jaloulid@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 19:14:11 by iel-asef          #+#    #+#             */
/*   Updated: 2026/07/26 00:48:48 by jaloulid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

// Forward declaration for function overloading
std::string buildPath(const std::string& requestPath,
                      const LocationConfig& loc,
                      bool appendIndex);

LocationConfig ServerConfig::matchLocation(const std::string& path) const
{
    if (locations.empty())
        return LocationConfig();

    LocationConfig best;
    bool found = false;

    for (size_t i = 0; i < locations.size(); i++)
    {
        const LocationConfig& loc = locations[i];

        bool match =
            (path == loc.path) ||
            (loc.path != "/" &&
             path.compare(0, loc.path.length(), loc.path) == 0 &&
             (path.size() == loc.path.length() ||
              path[loc.path.length()] == '/'));

        if (match && (!found || loc.path.length() > best.path.length()))
        {
            best = loc;
            found = true;
        }
    }

    if (found)
        return best;

    for (size_t i = 0; i < locations.size(); i++)
        if (locations[i].path == "/")
            return locations[i];

    return LocationConfig();
}

std::string redirectResponse(int code, const std::string& url)
{
    std::vector<std::string> headers;

    headers.push_back("Location: " + url);

    return buildResponse(code, "", "text/plain", headers);
}

std::string normalizePath(const std::string& path)
{
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;

    while (std::getline(ss, part, '/'))
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
    for (size_t i = 0; i < parts.size(); i++)
    {
        result += parts[i];
        if (i + 1 < parts.size())
            result += "/";
    }

    return result;
}
std::string buildPath(const std::string& requestPath,
                      const LocationConfig& loc)
{
    return buildPath(requestPath, loc, false);
}

std::string buildPath(const std::string& requestPath,
                      const LocationConfig& loc,
                      bool appendIndex)
{
    std::string path = loc.root;

    if (!path.empty() && path[path.size() - 1] != '/')
        path += '/';

    std::string rel = requestPath;

    size_t q = rel.find('?');
    if (q != std::string::npos)
        rel = rel.substr(0, q);

    if (loc.path != "/" &&
        rel.compare(0, loc.path.length(), loc.path) == 0 &&
        (rel.size() == loc.path.size() || rel[loc.path.size()] == '/'))
    {
        rel = rel.substr(loc.path.length());
    }

    while (rel.size() > 1 && rel[0] == '/')
        rel.erase(0, 1);

    if (rel.empty())
        rel = "/";

    path += rel;

    if (appendIndex)
    {
        if (path[path.size() - 1] != '/')
            path += '/';

        if (!loc.index.empty())
            path += loc.index;
        else
            path += "index.html";
    }

    return path;
}