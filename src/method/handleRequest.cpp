/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleRequest.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/17 13:44:30 by iel-asef          #+#    #+#             */
/*   Updated: 2026/07/06 14:55:28 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

static bool hasValidSessionCookie(const HttpRequest& req)
{
    std::map<std::string, std::string>::const_iterator it =
        req.headers.find("Cookie");

    if (it == req.headers.end())
        return false;

    return it->second.find("SESSIONID=") != std::string::npos;
}

static std::string generateSessionId()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    std::ostringstream out;
    out << tv.tv_sec << tv.tv_usec << getpid() << std::rand();
    return out.str();
}

static std::string attachSessionCookie(const HttpRequest& req,
                                       const std::string& response)
{
    if (hasValidSessionCookie(req))
        return response;

    size_t headerEnd = response.find("\r\n\r\n");

    if (headerEnd == std::string::npos)
        return response;

    std::string cookieHeader =
        "Set-Cookie: SESSIONID=" + generateSessionId() +
        "; Path=/; Max-Age=3600; HttpOnly";

    return response.substr(0, headerEnd) + "\r\n" + cookieHeader +
           response.substr(headerEnd);
}

 bool isInsideRoot(const std::string& path, const std::string& root)
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

std::string handleDELETE(const HttpRequest& req, const ServerConfig& serv)
{
    if (req.path.empty())
        return errorResponse(400, "", &serv);

    if (req.path.find("..") != std::string::npos)
        return errorResponse(403, "", &serv);

    std::string cleanPath = normalizePath(req.path);

    LocationConfig loc = serv.matchLocation(cleanPath);

    std::string target;
    if (loc.allow_upload && !loc.upload_path.empty())
    {
        std::string filename = cleanPath;

        size_t slash = filename.find_last_of('/');
        if (slash != std::string::npos)
            filename = filename.substr(slash + 1);

        if (filename.empty())
            return errorResponse(400, "", &serv);

        if (filename.find("..") != std::string::npos ||
            filename.find('/') != std::string::npos)
            return errorResponse(403, "", &serv);

        target = loc.upload_path;

        if (!target.empty() && target[target.size() - 1] != '/')
            target += '/';

        target += filename;
    }
    else
    {
        target = buildPath(cleanPath, loc);
    }

    if (!fileExists(target))
        return errorResponse(404, "", &serv);

    if (isDirectory(target) || isSymlink(target))
        return errorResponse(403, "", &serv);

    if (std::remove(target.c_str()) != 0)
        return errorResponse(500, "", &serv);

    return buildResponse(204, "", "");
}

std::string dispatchRequest(const HttpRequest& req ,const ServerConfig& Serv)
{
    std::string cleanPath = normalizePath(req.path);
    LocationConfig loc = Serv.matchLocation(cleanPath);

    if (req.path.find("%00") != std::string::npos ||
        req.query_string.find("%00") != std::string::npos)
        return attachSessionCookie(req, errorResponse(400, "", &Serv));

    if (cleanPath.size() > 8192 || req.query_string.size() > 8192)
        return attachSessionCookie(req, errorResponse(414, "", &Serv));

    if (loc.path.empty())//added this to fix unmatched URL path was returning 405 instead of 404
    return attachSessionCookie(req, errorResponse(404, "", &Serv));

    if (loc.internal)
        return attachSessionCookie(req, errorResponse(404, "", &Serv));
    
    if (loc.redirect_code != 0)
        return attachSessionCookie(req, redirectResponse(loc.redirect_code, loc.redirect));
        
    if (loc.root.empty())
        return attachSessionCookie(req, errorResponse(500, "", &Serv));

    std::string allowHeader = buildAllowHeader(loc);

    if (!isMethodAllowed(loc, req.method))
        return attachSessionCookie(req, errorResponse(405, allowHeader, &Serv));

    HttpRequest cleanReq = req;
    bool isHeadRequest = (req.method == "HEAD");

    if (isHeadRequest)
        cleanReq.method = "GET";

    if (!req.path.empty() && req.path[req.path.size() - 1] == '/' && cleanPath != "/") 
        cleanReq.path = cleanPath + "/";
    else
        cleanReq.path = cleanPath;

    if (cleanReq.method == "GET"){
        std::string response = handleRequest(cleanReq, loc, &Serv);

        if (!isHeadRequest)
            return attachSessionCookie(req, response);

        size_t bodyPos = response.find("\r\n\r\n");

        if (bodyPos == std::string::npos)
            return attachSessionCookie(req, response);

        return attachSessionCookie(req, response.substr(0, bodyPos + 4));
    }

    if (cleanReq.method == "POST")
        return attachSessionCookie(req, handlePOST(cleanReq, Serv));
    
    if (cleanReq.method == "DELETE")
        return attachSessionCookie(req, handleDELETE(cleanReq, Serv));
    
    return attachSessionCookie(req, errorResponse(501, "", &Serv));
}