/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handlePost.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 21:58:29 by iel-asef          #+#    #+#             */
/*   Updated: 2026/07/02 21:58:30 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"


std::string extractFilename(const std::string& body)
{
    size_t pos = body.find("filename=\"");
    if (pos == std::string::npos)
        return "";

    pos += 10;
    size_t end = body.find("\"", pos);

    return body.substr(pos, end - pos);
}

std::string extractContent(const std::string& body)
{
    size_t start = body.find("\r\n\r\n");
    if (start == std::string::npos)
        return "";

    start += 4;

    size_t end = body.rfind("--");
    if (end == std::string::npos)
        return body.substr(start);

    return body.substr(start, end - start);
}

std::string handlePOST(const HttpRequest& req, const ServerConfig& serv)
{
    // if (!loc.allow_post)
    //     return errorResponse(403);

    if (req.path.empty())
        return errorResponse(400, "", &serv);

    if (req.path.find("..") != std::string::npos)
        return errorResponse(403, "", &serv);

    std::string cleanPath = normalizePath(req.path);

    if (cleanPath.find("..") != std::string::npos)
        return errorResponse(403, "", &serv);

    std::map<std::string, std::string>::const_iterator it =
        req.headers.find("Content-Length");

    if (it == req.headers.end())
        return errorResponse(411, "", &serv);

    size_t contentLength = std::atoi(it->second.c_str());

    if (serv.client_max_body_size > 0 && contentLength > serv.client_max_body_size)
        return errorResponse(413, "", &serv);

    if (req.body.size() != contentLength)
        return errorResponse(400, "", &serv);

    std::string baseDir;
    // if (loc.allow_upload)
    // {
    //     if (loc.upload_path.empty())
    //         return errorResponse(500);
    //     // baseDir = loc.upload_path;
    // }
    // else
    // {
    //     baseDir = buildPath(cleanPath, loc);
    // }

    // if (loc.allow_upload && !isDirectory(baseDir))
    //     return errorResponse(500);

    std::map<std::string, std::string>::const_iterator ct =
        req.headers.find("Content-Type");

    if (ct != req.headers.end() &&
        ct->second.find("multipart/form-data") != std::string::npos)
    {
        std::string filename = extractFilename(req.body);
        std::string content  = extractContent(req.body);

        if (filename.empty() || content.empty())
            return errorResponse(400, "", &serv);

        if (filename.find("/") != std::string::npos ||
            filename.find("..") != std::string::npos)
            return errorResponse(403, "", &serv);

        std::string path = baseDir;
        if (!path.empty() && path[path.size() - 1] != '/')
            path += "/";
        path += filename;

        if (isDirectory(path))
            return errorResponse(403, "", &serv);

        std::ofstream file(path.c_str(), std::ios::binary);
        if (!file.is_open())
            return errorResponse(500, "", &serv);

        file.write(content.c_str(), content.size());
        file.close();

        return buildResponse(201, "", "");
    }

    std::string name;
    size_t slash = cleanPath.find_last_of('/');
    if (slash == std::string::npos)
        name = cleanPath;
    else
        name = cleanPath.substr(slash + 1);

    if (name.empty())
        return errorResponse(400, "", &serv);

    if (name.find("..") != std::string::npos || name.find("/") != std::string::npos)
        return errorResponse(403, "", &serv);

    std::string target = baseDir;
    if (!target.empty() && target[target.size() - 1] != '/')
        target += "/";
    target += name;

    if (isDirectory(target))
        return errorResponse(403, "", &serv);

    std::ofstream file(target.c_str(), std::ios::binary);
    if (!file.is_open())
        return errorResponse(500, "", &serv);

    file.write(req.body.c_str(), contentLength);
    file.close();

    return buildResponse(201, "", "");
}

