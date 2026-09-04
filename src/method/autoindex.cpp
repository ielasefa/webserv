/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   autoindex.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 21:40:33 by iel-asef          #+#    #+#             */
/*   Updated: 2026/07/02 21:40:37 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

static std::string escapeHTML(const std::string& str) //XSS (Cross-Site Scripting)
{
    std::string out;

    for (size_t i = 0; i < str.size(); i++)
    {
        switch (str[i])
        {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += str[i];
        }
    }
    return out;
}

std::vector<std::string> readDirectory(const std::string& path)
{
    std::vector<std::string> files;

    DIR *dir = opendir(path.c_str());
    if (!dir)
        return files;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;

        if (name == "." || name == "..")
            continue;

        files.push_back(name);
    }

    closedir(dir);

    std::sort(files.begin(), files.end());

    return files;
}

std::string generateAutoIndex(const std::string& requestPath, const std::string& directoryPath, const std::vector<std::string>& files)
{
    std::string base = requestPath;

    if (!base.empty() && base[base.size() - 1] != '/')
        base += "/";

    std::string html;

    html += "<html>";
    html += "<head>";
    html += "<title>Index of " + escapeHTML(requestPath) + "</title>";
    html += "</head>";
    html += "<body>";

    html += "<h1>Index of " + escapeHTML(requestPath) + "</h1>";
    html += "<hr>";
    html += "<ul>";

    if (requestPath != "/")
        html += "<li><a href=\"../\">../</a></li>";

    for (size_t i = 0; i < files.size(); i++)
    {
        std::string fileName = files[i];
        std::string fullPath = directoryPath;

        if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/')
            fullPath += "/";

        fullPath += fileName;

        struct stat st;
        bool isDir = false;

        if (stat(fullPath.c_str(), &st) == 0)
            isDir = S_ISDIR(st.st_mode);

        html += "<li><a href=\"";
        html += base + fileName;

        if (isDir)
            html += "/";

        html += "\">";
        html += escapeHTML(fileName);

        if (isDir)
            html += "/";

        html += "</a></li>";
    }

    if (files.empty())
        html += "<li><i>Directory is empty</i></li>";

    html += "</ul>";
    html += "<hr>";
    html += "</body>";
    html += "</html>";

    return buildResponse(200, html, "text/html");
}