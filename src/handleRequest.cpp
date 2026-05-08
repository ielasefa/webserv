/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleRequest.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/17 13:44:30 by iel-asef          #+#    #+#             */
/*   Updated: 2026/04/17 13:48:51 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"
std::string handleDELETE(const Request& req, const Location& loc)
{
    if (!loc.allow_delete)
        return errorResponse(403);

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
    Location loc = matchLocation(req.path);

    if (!isMethodAllowed(loc, req.method))
        return errorResponse(405);

    if (req.method == "GET")
        return handleRequest(req, loc);

    if (req.method == "POST")
        return handlePOST(req, loc);

    if (req.method == "DELETE")
        return handleDELETE(req, loc);

    return errorResponse(405);
}