/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleRequest.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/17 13:44:30 by iel-asef          #+#    #+#             */
/*   Updated: 2026/07/02 21:54:34 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

static bool isInsideRoot(const std::string& path, const std::string& root)
{
    char realRoot[PATH_MAX];
    char realPath[PATH_MAX];

    if (!realpath(root.c_str(), realRoot))
        return false;

    if (realpath(path.c_str(), realPath))
    {
        std::string p(realPath);
        std::string r(realRoot);
        
        if (r[r.length() - 1] != '/')
            r += "/";
        
        return (p == realRoot) || (p.find(r) == 0);
    }
    else
    {
        size_t lastSlash = path.rfind('/');
        if (lastSlash == std::string::npos)
            return false;
        
        std::string parentDir = path.substr(0, lastSlash);
        if (parentDir.empty())
            parentDir = "/";
        
        if (realpath(parentDir.c_str(), realPath))
        {
            std::string p(realPath);
            std::string r(realRoot);
            
            if (r[r.length() - 1] != '/')
                r += "/";
            
            return (p == realRoot) || (p.find(r) == 0);
        }
    }

    return false;
}

std::string handleDELETE(const HttpRequest& req, const LocationConfig& loc,
                         const ServerConfig* config)
{
    // if (!loc.allow_delete)
    //     return errorResponse(403);

    std::string cleanPath = normalizePath(req.path);

    if (cleanPath.find("..") != std::string::npos)
        return errorResponse(403, "", config);

    std::string fullPath = buildPath(cleanPath, loc);

    if (!isInsideRoot(fullPath, loc.root))
        return errorResponse(403, "", config);

    if (isSymlink(fullPath))
        return errorResponse(403, "", config);

    struct stat st;
    if (stat(fullPath.c_str(), &st) != 0)
        return errorResponse(404, "", config);

    if (S_ISDIR(st.st_mode))
        return errorResponse(403, "", config);

    if (access(fullPath.c_str(), W_OK) != 0)
        return errorResponse(403, "", config);

    if (std::remove(fullPath.c_str()) != 0)
        return errorResponse(500, "", config);

    return buildResponse(204, "", "");  
}

std::string dispatchRequest(const HttpRequest& req ,const ServerConfig& Serv)
{
    std::string cleanPath = normalizePath(req.path);
    LocationConfig loc = Serv.matchLocation(cleanPath);
    
    if (loc.internal)
        return errorResponse(404, "", &Serv);
    
    if (loc.redirect_code != 0)
        return redirectResponse(loc.redirect_code, loc.redirect);
        
    if (loc.root.empty())
        return errorResponse(500, "", &Serv);
    HttpRequest cleanReq = req;
    cleanReq.path = cleanPath;
    std::string allowHeader = buildAllowHeader(loc);
    
    if (!isMethodAllowed(loc, cleanReq.method))
        return errorResponse(405, allowHeader, &Serv);

    if (cleanReq.method == "GET"){
        return handleRequest(cleanReq, loc, &Serv);
    }

    if (cleanReq.method == "POST")
        return handlePOST(cleanReq, Serv);
    
    if (cleanReq.method == "DELETE")
        return handleDELETE(cleanReq, loc, &Serv);
    
    return errorResponse(405, allowHeader, &Serv);
}