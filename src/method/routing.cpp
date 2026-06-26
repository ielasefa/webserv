/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   routing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 19:14:11 by iel-asef          #+#    #+#             */
/*   Updated: 2026/06/06 14:38:47 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

LocationConfig ServerConfig::matchLocation(const std::string& path) const
{
    if (locations.empty())
        return LocationConfig();

    LocationConfig best;
    bool matched = false;

    for (size_t i = 0; i < locations.size(); i++)
    {
        const LocationConfig& loc = locations[i];
        bool isMatch = false;

        if (loc.path == "/")
            isMatch = true;
        else if (path == loc.path || path.find(loc.path + "/") == 0)
            isMatch = true;

        if (isMatch && (!matched || loc.path.length() > best.path.length()))
        {
            best = loc;
            matched = true;
        }
    }

    if (!matched)
        return locations[0];

    return best;
}

std::string normalizePath(const std::string& path)
{
    std::string result;
    bool lastWasSlash = false;

    if (path.empty() || path[0] != '/')
        result += '/';

    for (size_t i = 0; i < path.size(); i++)
    {
        if (path[i] == '/')
        {
            if (!lastWasSlash)
            {
                result += '/';
                lastWasSlash = true;
            }
        }
        else
        {
            result += path[i];
            lastWasSlash = false;
        }
    }

    if (result.size() > 1 && result[result.size() - 1] == '/')
        result.erase(result.size() - 1);

    return result;
}

std::string buildPath(const std::string& requestPath, const LocationConfig& loc)
{
    return buildPath(requestPath, loc, false);
}

std::string buildPath(const std::string& requestPath, const LocationConfig& loc, bool appendIndex)
{
    std::string clean = requestPath;

    if (loc.path != "/" && (clean == loc.path || clean.find(loc.path + "/") == 0))
        clean.erase(0, loc.path.length());
    else if (loc.path == "/" && !clean.empty() && clean[0] == '/')
        clean.erase(0, 1);

    if (clean.empty())
        clean = "/";

    if (clean[0] != '/')
        clean = "/" + clean;

    std::string root = loc.root;
    if (!root.empty() && root[root.size() - 1] == '/' && clean[0] == '/')
        root.erase(root.size() - 1);

    std::string base = root + clean;

    if (appendIndex)
    {
        if (!base.empty() && base[base.size() - 1] != '/')
            base += "/";
        if (!loc.index.empty())
            base += loc.index;
        else
            base += "index.html";
    }

    return base;
}