/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 19:43:11 by iel-asef          #+#    #+#             */
/*   Updated: 2026/07/02 20:11:48 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

std::string statusMessage(int code)
{
    switch (code)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";

        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";

        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 411: return "Length Requiredddddd";//
        case 413: return "Payload Too Large";

        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";

        default:
        {
            std::ostringstream ss;
            ss << code << " Error";
            return ss.str();
        }
    }
}


static std::string defaultErrorHTML(int code)
{
    std::ostringstream html;

    html << "<!DOCTYPE html>"
         << "<html>"
         << "<head>"
         << "<title>" << statusMessage(code) << "</title>"
         << "</head>"
         << "<body style=\"font-family:Arial;text-align:center;padding:40px;\">"
         << "<h1>" << statusMessage(code) << "</h1>"
         << "<p>The server could not display the requested error page.</p>"
         << "</body>"
         << "</html>";

    return html.str();
}

std::string errorResponse(int code,
                          const std::string& allowHeader,
                          const ServerConfig* config)
{
    std::vector<std::string> headers;

    if (!allowHeader.empty())
        headers.push_back("Allow: " + allowHeader);

    std::string body;

    if (config)
    {
        std::map<int, std::string>::const_iterator it =
            config->error_pages.find(code);

        if (it != config->error_pages.end())
        {
            std::string path = config->root;

            if (!path.empty() && path[path.size() - 1] != '/')
                path += '/';

            std::string uri = it->second;

            if (!uri.empty() && uri[0] == '/')
                uri.erase(0, 1);

            path += uri;

            if (isFile(path))
            {
                body = readFile(path);

                if (body.empty())
                    body = defaultErrorHTML(code);
            }
            else
                body = defaultErrorHTML(code);
        }
    }
    else
        body = defaultErrorHTML(code);
        
    return buildResponse(code, body, "text/html", headers);
}