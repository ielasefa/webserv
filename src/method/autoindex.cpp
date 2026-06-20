/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   autoindex.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 19:50:57 by iel-asef          #+#    #+#             */
/*   Updated: 2026/04/14 14:11:23 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/webserv.hpp"

std::vector<std::string> readDirectory(const std::string& path)
{
    std::vector<std::string> files;

    DIR* dir = opendir(path.c_str());
    if (!dir)
        return files;

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        files.push_back(name);
    }

    closedir(dir);
    return files;
}

std::string generateAutoIndex(const std::string& requestPath,
                              const std::vector<std::string>& files)
{
    std::string path = requestPath;

    if (!path.empty() && path[path.size() - 1] != '/')
        path += "/";

    std::string html;

    html += "<html><head><title>Index</title></head><body>";
    html += "<h1>Index of " + requestPath + "</h1>";
    html += "<ul>";

    for (size_t i = 0; i < files.size(); i++)
    {
        html += "<li><a href=\"" + path + files[i] + "\">";
        html += files[i];
        html += "</a></li>";
    }

    html += "</ul></body></html>";

    return buildResponse(200, html, "text/html");
}