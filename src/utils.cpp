/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 19:14:08 by iel-asef          #+#    #+#             */
/*   Updated: 2026/04/14 19:14:09 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"
#include <sys/stat.h>

bool isFile(const std::string& path)
{
    struct stat s;
    return (stat(path.c_str(), &s) == 0 && S_ISREG(s.st_mode));
}


bool fileExists(const std::string& path)
{
    struct stat s;

    if (stat(path.c_str(), &s) == 0)
        return true;

    return false;
}

bool isDirectory(const std::string& path)
{
    struct stat s;
    return (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode));
}

std::string readFile(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string sizeToString(size_t value)
{
    std::stringstream stream;
    stream << value;
    return stream.str();
}

std::string readBody(int fd, size_t contentLength)
{
    std::string body;
    char buffer[1024];
    size_t total = 0;

    while (total < contentLength)
    {
        int bytes = recv(fd, buffer, 1024, 0);

        if (bytes <= 0)
            break;

        body.append(buffer, bytes);
        total += bytes;
    }

    return body;
}

// std::string getMimeType(const std::string& path)
// {
//     if (path.find(".html") != std::string::npos)
//         return "text/html";
//     if (path.find(".css") != std::string::npos)
//         return "text/css";
//     if (path.find(".js") != std::string::npos)
//         return "application/javascript";
//     if (path.find(".png") != std::string::npos)
//         return "image/png";
//     if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos)
//         return "image/jpeg";

//     return "text/plain";
// }