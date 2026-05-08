/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   routing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 19:14:11 by iel-asef          #+#    #+#             */
/*   Updated: 2026/04/14 19:33:39 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

std::vector<Location> locations;

void initLocations()
{
    Location l1;
    l1.path = "/";
    l1.root = "www";
    l1.index = "index.html";
    l1.autoindex = false;
    l1.allow_post = false;
    l1.allow_delete = false;
    l1.allow_upload = false;
    l1.client_max_body_size = 0;
    l1.allowed_methods.push_back("GET");

    Location l2;
    l2.path = "/images";
    l2.root = "www/images";
    l2.index = "index.html";
    l2.autoindex = true;
    l2.allow_post = false;
    l2.allow_delete = false;
    l2.allow_upload = false;
    l2.client_max_body_size = 0;
    l2.allowed_methods.push_back("GET");

    Location l3;
    l3.path = "/upload";
    l3.root = "www/upload";
    l3.index = "index.html";
    l3.autoindex = false;
    l3.allow_post = true;
    l3.allow_delete = false;
    l3.allow_upload = true;
    l3.upload_path = "www/upload";
    l3.client_max_body_size = 0;
    l3.allowed_methods.push_back("GET");
    l3.allowed_methods.push_back("POST");

    locations.push_back(l1);
    locations.push_back(l2);
    locations.push_back(l3);
}

Location matchLocation(const std::string& requestPath)
{
    Location best = locations[0];

    for (size_t i = 0; i < locations.size(); i++)
    {
        const Location& loc = locations[i];

        if (requestPath == loc.path || requestPath.find(loc.path + "/") == 0)
        {
            if (loc.path.length() > best.path.length())
                best = loc;
        }
    }

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

std::string buildPath(const std::string& requestPath, const Location& loc)
{
    std::string clean = requestPath;

    if (clean == "/")
        return loc.root + "/" + loc.index;

    if (clean.find(loc.path) == 0)
        clean = clean.substr(loc.path.length());

    if (clean.empty())
        clean = "/";

    return loc.root + clean;
}