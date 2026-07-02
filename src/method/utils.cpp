/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 19:14:08 by iel-asef          #+#    #+#             */
/*   Updated: 2026/07/02 21:41:39 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

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

bool isMethodAllowed(const LocationConfig& loc, const std::string& method)
{
    for (size_t i = 0; i < loc.allowed_methods.size(); i++)
    {
        if (loc.allowed_methods[i] == method)
            return true;
    }
    return false;
}

std::string buildAllowHeader(const LocationConfig& loc)
{
    if (loc.allowed_methods.empty())
        return "";

    std::string header;

    for (size_t i = 0; i < loc.allowed_methods.size(); i++)
    {
        if (i > 0)
            header += ", ";
        header += loc.allowed_methods[i];
    }

    return header;
}

