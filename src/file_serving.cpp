/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   file_serving.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 19:50:53 by iel-asef          #+#    #+#             */
/*   Updated: 2026/04/15 11:41:37 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

std::string serveFile(const std::string& path)
{
    std::string content = readFile(path);

    if (content.empty())
        return errorResponse(404);
    std::string res;
    res += "HTTP/1.1 200 OK\r\n";
    res += "Content-Length: " + sizeToString(content.size()) + "\r\n";
    res += "Content-Type: text/html\r\n";
    res += "Connection: close\r\n\r\n";
    res += content;

    return res;
}
std::string redirect301(const std::string& newPath)
{
    std::string res;
    res += "HTTP/1.1 301 Moved Permanently\r\n";
    res += "Location: " + newPath + "\r\n";
    res += "Content-Length: 0\r\n";
    res += "Connection: close\r\n\r\n";
    return res;
}

std::string handleRequest(const std::string& requestPath)
{
    // 🔴 Security
    if (requestPath.find("..") != std::string::npos)
        return errorResponse(403);

    // 🟢 Normalize
    std::string cleanPath = normalizePath(requestPath);

    // 🟢 Routing
    Location loc = matchLocation(cleanPath);

    // 🟢 Build real path
    std::string path = buildPath(cleanPath, loc);


    // 🟢 FILE → serve مباشرة
    if (isFile(path))
        return serveFile(path);

    // 🟢 DIRECTORY
    if (isDirectory(path))
    {
        // 🔥 Trailing slash redirect
        if (cleanPath[cleanPath.size() - 1] != '/')
            return redirect301(cleanPath + "/");

        // ✔ index.html path safe
        std::string index = path;
        if (path[path.size() - 1] != '/')
            index += "/";
        index += "index.html";

        // ✔ serve index if exists
        if (isFile(index))
            return serveFile(index);

        // ❌ autoindex off
        if (!loc.autoindex)
            return errorResponse(403);

        // ✔ autoindex
        std::vector<std::string> files = readDirectory(path);

        return generateAutoIndex(cleanPath, files);
    }

    // 🔴 Not found
    return errorResponse(404);
}