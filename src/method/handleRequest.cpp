/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleRequest.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/17 13:44:30 by iel-asef          #+#    #+#             */
/*   Updated: 2026/06/06 14:39:53 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/webserv.hpp"
std::string handleDELETE(const Request& req, const LocationConfig& loc)
{
    // if (!loc.allow_delete)
    //     return errorResponse(403);

    std::string cleanPath = normalizePath(req.path);

    if (cleanPath.find("..") != std::string::npos)
        return errorResponse(403);

    std::string path = buildPath(cleanPath, loc);

    if (!fileExists(path))
        return errorResponse(404);

    if (isDirectory(path))
        return errorResponse(403);

    if (access(path.c_str(), W_OK) != 0)
        return errorResponse(403);

    if (remove(path.c_str()) != 0)
        return errorResponse(500);

    return buildResponse(204, "", "");
}

std::string dispatchRequest(const Request& req)
{
    std::string cleanPath = normalizePath(req.path);
    LocationConfig loc = matchLocation(cleanPath);

    // if (loc.root.empty() && loc.path.empty())
    //     return errorResponse(500);
    // Ayoub commented this 7it dima katkhdm
    Request cleanReq = req;
    cleanReq.path = cleanPath;
    // std::cout << "--------\n" << cleanReq.method << "\n-------" << std::endl;
    std::string allowHeader = buildAllowHeader(loc);

    if (!isMethodAllowed(loc, cleanReq.method))
        return errorResponse(405, allowHeader);

    if (cleanReq.method == "GET")
        return handleRequest(cleanReq, loc);

    if (cleanReq.method == "POST")
        return errorResponse(405, allowHeader);

    if (cleanReq.method == "DELETE")
        return handleDELETE(cleanReq, loc);

    return errorResponse(405, allowHeader);
}