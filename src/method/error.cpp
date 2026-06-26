/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 19:48:54 by iel-asef          #+#    #+#             */
/*   Updated: 2026/04/13 19:48:55 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

static std::string defaultErrorHTML(int code)
{
    if (code == 400)
        return "<html><body><h1>400 Bad Request</h1></body></html>";

    if (code == 403)
        return "<html><body><h1>403 Forbidden</h1></body></html>";

    if (code == 404)
        return "<html><body><h1>404 Not Found</h1></body></html>";

    if (code == 405)
        return "<html><body><h1>405 Method Not Allowed</h1></body></html>";

    if (code == 500)
        return "<html><body><h1>500 Internal Server Error</h1></body></html>";

    return "<html><body><h1>Unknown Error</h1></body></html>";
}

std::string statusMessage(int code)
{
    switch (code)
    {
        case 200: return "200 OK";
        case 201: return "201 Created";
        case 204: return "204 No Content";
        case 301: return "301 Moved Permanently";
        case 400: return "400 Bad Request";
        case 403: return "403 Forbidden";
        case 404: return "404 Not Found";
        case 405: return "405 Method Not Allowed";
        case 411: return "411 Length Required";
        case 413: return "413 Payload Too Large";
        case 500: return "500 Internal Server Error";
    }

    return "500 Internal Server Error";
}

std::string errorResponse(int code, const std::string& allowHeader)
{
    std::string body = defaultErrorHTML(code);
    std::vector<std::string> headers;

    if (!allowHeader.empty())
        headers.push_back(allowHeader);

    return buildResponse(code, body, "text/html", headers);
}