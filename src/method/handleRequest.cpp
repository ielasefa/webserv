/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleRequest.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/17 13:44:30 by iel-asef          #+#    #+#             */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"
#include <cstdlib>
#include <ctime>
#include <set>

static std::set<std::string> g_validSessions;

static std::string toLower(const std::string& value)
{
    std::string result = value;

    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(result[i])));
    }
    return result;
}

static bool getHeaderValue(const HttpRequest& req, const std::string& wanted, std::string& value)
{
    std::string target = toLower(wanted);

    for (std::map<std::string, std::string>::const_iterator it
             = req.headers.begin();
         it != req.headers.end(); ++it)
    {
        if (toLower(it->first) == target)
        {
            value = it->second;
            return true;
        }
    }
    return false;
}

static std::string trimSessionValue(const std::string& value)
{
    size_t begin = 0;
    size_t end = value.size();

    while (begin < end
           && (value[begin] == ' ' || value[begin] == '\t'))
        ++begin;
    while (end > begin
           && (value[end - 1] == ' ' || value[end - 1] == '\t'))
        --end;
    return value.substr(begin, end - begin);
}

static bool extractSessionId(const HttpRequest& req, std::string& sessionId)
{
    std::string cookie;
    size_t start;

    if (!getHeaderValue(req, "Cookie", cookie))
        return false;
    start = 0;
    while (start < cookie.size())
    {
        size_t end = cookie.find(';', start);
        std::string part = cookie.substr(start, end - start);
        size_t equal;

        part = trimSessionValue(part);
        equal = part.find('=');
        if (equal != std::string::npos
            && trimSessionValue(part.substr(0, equal)) == "SESSIONID")
        {
            sessionId = trimSessionValue(part.substr(equal + 1));
            return !sessionId.empty();
        }
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return false;
}

static bool hasValidSessionCookie(const HttpRequest& req)
{
    std::string sessionId;

    if (!extractSessionId(req, sessionId))
        return false;
    return g_validSessions.find(sessionId) != g_validSessions.end();
}

static std::string generateSessionId()
{
    static unsigned long counter = 0;
    std::string id;

    do
    {
        std::ostringstream out;

        out << std::time(NULL) << "_" << std::clock() << "_"
            << std::rand() << "_" << counter++;
        id = out.str();
    }
    while (g_validSessions.find(id) != g_validSessions.end());
    return id;
}

static std::string attachSessionCookie(const HttpRequest& req, const std::string& response)
{
    size_t headerEnd;
    std::string sessionId;
    std::string cookieHeader;

    if (hasValidSessionCookie(req))
        return response;
    headerEnd = response.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return response;
    sessionId = generateSessionId();
    g_validSessions.insert(sessionId);
    cookieHeader = "Set-Cookie: SESSIONID=" + sessionId
        + "; Path=/; Max-Age=3600; HttpOnly";
    return response.substr(0, headerEnd) + "\r\n" + cookieHeader
        + response.substr(headerEnd);
}

static void addPathPart(std::vector<std::string>& parts, const std::string& part,
                        bool absolute)
{
    if (part == "..")
    {
        if (!parts.empty() && parts[parts.size() - 1] != "..")
            parts.pop_back();
        else if (!absolute)
            parts.push_back(part);
    }
    else if (!part.empty() && part != ".")
        parts.push_back(part);
}

static std::string joinPathParts(const std::vector<std::string>& parts, bool absolute)
{
    std::string result;

    if (absolute)
        result = "/";
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (!result.empty() && result[result.size() - 1] != '/')
            result += "/";
        result += parts[i];
    }
    if (result.empty())
        return absolute ? "/" : ".";
    return result;
}

static std::string normalizeFilesystemPath(const std::string& path)
{
    bool absolute = !path.empty() && path[0] == '/';
    std::vector<std::string> parts;
    size_t start = 0;

    while (start < path.size())
    {
        while (start < path.size() && path[start] == '/')
            ++start;
        if (start == path.size())
            break;
        size_t end = path.find('/', start);

        addPathPart(parts, path.substr(start, end - start), absolute);
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return joinPathParts(parts, absolute);
}

bool isInsideRoot(const std::string& path, const std::string& root)
{
    std::string cleanPath;
    std::string cleanRoot;

    if (path.empty() || root.empty())
        return false;
    cleanPath = normalizeFilesystemPath(path);
    cleanRoot = normalizeFilesystemPath(root);
    if (cleanPath == cleanRoot)
        return true;
    if (cleanRoot == "/")
        return !cleanPath.empty() && cleanPath[0] == '/';
    if (cleanRoot == ".")
    {
        return cleanPath != ".."
            && cleanPath.compare(0, 3, "../") != 0
            && (cleanPath.empty() || cleanPath[0] != '/');
    }
    cleanRoot += "/";
    return cleanPath.compare(0, cleanRoot.size(), cleanRoot) == 0;
}

static int buildDeleteTarget(const std::string& cleanPath, const LocationConfig& loc, std::string& target,
                            std::string& securityRoot)
{
    if (loc.allow_upload && !loc.upload_path.empty())
    {
        size_t slash = cleanPath.find_last_of('/');
        std::string filename = cleanPath;

        if (slash != std::string::npos)
            filename = cleanPath.substr(slash + 1);
        if (filename.empty())
            return 400;
        if (filename.find('/') != std::string::npos)
            return 403;
        target = loc.upload_path;
        if (!target.empty() && target[target.size() - 1] != '/')
            target += "/";
        target += filename;
        securityRoot = loc.upload_path;
    }
    else
    {
        target = buildPath(cleanPath, loc);
        securityRoot = loc.root;
    }
    return 0;
}

static int removeTarget(const std::string& target)
{
    if (std::remove(target.c_str()) == 0)
        return 204;
    if (errno == EACCES || errno == EPERM)
        return 403;
    if (errno == ENOENT)
        return 404;
    return 500;
}

std::string handleDELETE(const HttpRequest& req, const ServerConfig& serv)
{
    std::string cleanPath;
    LocationConfig loc;
    std::string target;
    std::string securityRoot;
    int status;

    if (req.path.empty())
        return errorResponse(400, "", &serv);
    cleanPath = normalizePath(req.path);
    loc = serv.matchLocation(cleanPath);
    if (loc.path.empty())
        return errorResponse(404, "", &serv);
    status = buildDeleteTarget(cleanPath, loc, target, securityRoot);
    if (status != 0)
        return errorResponse(status, "", &serv);
    if (securityRoot.empty() || !isInsideRoot(target, securityRoot))
        return errorResponse(403, "", &serv);
    if (!fileExists(target))
        return errorResponse(404, "", &serv);
    if (isDirectory(target) || isSymlink(target))
        return errorResponse(403, "", &serv);
    status = removeTarget(target);
    if (status != 204)
        return errorResponse(status, "", &serv);
    return buildResponse(204, "", "");
}

static int hexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static bool percentDecode(const std::string& input, std::string& output)
{
    output.clear();
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (input[i] != '%')
        {
            output += input[i];
            continue;
        }
        if (i + 2 >= input.size())
            return false;
        int high = hexValue(input[i + 1]);
        int low = hexValue(input[i + 2]);

        if (high == -1 || low == -1)
            return false;
        char decoded = static_cast<char>((high << 4) | low);

        if (decoded == '\0')
            return false;
        output += decoded;
        i += 2;
    }
    return true;
}

static bool supportedMethod(const std::string& method)
{
    return method == "GET" || method == "POST"
        || method == "DELETE" || method == "HEAD";
}

static int validateRequest(const HttpRequest& req)
{
    std::string host;

    if (req.method.empty() || req.path.empty() || req.version.empty())
        return 400;
    if (req.version != "HTTP/1.1")
        return 505;
    if (!getHeaderValue(req, "Host", host) || host.empty())
        return 400;
    if (!supportedMethod(req.method))
        return 501;
    if (req.path.size() > 8192 || req.query_string.size() > 8192)
        return 414;
    return 0;
}

static int preparePath(const HttpRequest& req, std::string& decodedPath, std::string& cleanPath)
{
    if (!percentDecode(req.path, decodedPath))
        return 400;
    cleanPath = normalizePath(decodedPath);
    if (cleanPath.empty())
        return 400;
    if (cleanPath.size() > 8192)
        return 414;
    return 0;
}

static HttpRequest makeCleanRequest(const HttpRequest& req, const std::string& decodedPath, const std::string& cleanPath)
{
    HttpRequest cleanReq = req;

    cleanReq.path = cleanPath;
    if (!decodedPath.empty()
        && decodedPath[decodedPath.size() - 1] == '/'
        && cleanPath != "/"
        && cleanPath[cleanPath.size() - 1] != '/')
        cleanReq.path += "/";
    return cleanReq;
}

static std::string executeRequest(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& serv)
{
    if (req.method == "GET" || req.method == "HEAD")
        return handleRequest(req, loc, &serv);
    if (req.method == "POST")
        return handlePOST(req, serv);
    if (req.method == "DELETE")
        return handleDELETE(req, serv);
    return errorResponse(501, "", &serv);
}

static std::string routeRequest(const HttpRequest& req, const std::string& decodedPath, const std::string& cleanPath,
                                const ServerConfig& serv)
{
    LocationConfig loc = serv.matchLocation(cleanPath);
    std::string allowHeader;
    HttpRequest cleanReq;

    if (loc.path.empty() || loc.internal)
        return errorResponse(404, "", &serv);
    if (loc.redirect_code != 0)
        return redirectResponse(loc.redirect_code, loc.redirect);
    if (loc.root.empty())
        return errorResponse(500, "", &serv);
    allowHeader = buildAllowHeader(loc);
    if (!isMethodAllowed(loc, req.method))
        return errorResponse(405, allowHeader, &serv);
    cleanReq = makeCleanRequest(req, decodedPath, cleanPath);
    return executeRequest(cleanReq, loc, serv);
}

std::string dispatchRequest(const HttpRequest& req,
                            const ServerConfig& serv)
{
    std::string decodedPath;
    std::string cleanPath;
    std::string response;
    int status;

    status = validateRequest(req);
    if (status != 0)
        response = errorResponse(status, "", &serv);
    else
    {
        status = preparePath(req, decodedPath, cleanPath);
        if (status != 0)
            response = errorResponse(status, "", &serv);
        else
            response = routeRequest(req, decodedPath, cleanPath, serv);
    }
    return attachSessionCookie(req, response);
}
