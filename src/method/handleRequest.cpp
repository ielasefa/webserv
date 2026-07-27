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

    if (!loc.allow_delete)
        return errorResponse(405, "", &serv);

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
    
    if (loc.internal)
        return errorResponse(404, "", &Serv);
    
    if (loc.redirect_code != 0)
        return redirectResponse(loc.redirect_code, loc.redirect);
        
    if (loc.root.empty())
        return errorResponse(500, "", &Serv);
    HttpRequest cleanReq = req;

    if (!req.path.empty() && req.path[req.path.size() - 1] == '/' && cleanPath != "/") 
        cleanReq.path = cleanPath + "/";
    else
        cleanReq.path = cleanPath;
    
    std::string allowHeader = buildAllowHeader(loc);
    
    if (!isMethodAllowed(loc, cleanReq.method))
        return errorResponse(405, allowHeader, &Serv);
    
    size_t dotPos = cleanPath.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        
        std::string ext = cleanPath.substr(dotPos);
        if (loc.cgi_pass.find(ext) != loc.cgi_pass.end())
        {
            std::string scriptPath = buildPath(cleanPath, loc);
            if (!isFile(scriptPath))
                return errorResponse(404, "", &Serv);
            
            // CGIHandler cgi(cleanReq, scriptPath, loc.cgi_pass.at(ext));
            // std::cout << "cgi hnaaaaa" << std::endl;
            // return cgi.execute();
        }
    }
    
    if (cleanReq.method == "GET"){
        return handleRequest(cleanReq, loc, &Serv);
    }

    if (cleanReq.method == "POST")
        return handlePOST(cleanReq, Serv);
    
    if (cleanReq.method == "DELETE")
        return handleDELETE(cleanReq, Serv);
    
    return errorResponse(405, allowHeader, &Serv);
}