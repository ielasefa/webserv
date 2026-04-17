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

#include "webserv.hpp"

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


static std::string statusMessage(int code)
{
    if (code == 400)
        return "400 Bad Request";
    if (code == 403)
        return "403 Forbidden";
    if (code == 404)
        return "404 Not Found";
    if (code == 405)
        return "405 Method Not Allowed";
    if (code == 500)
        return "500 Internal Server Error";

    return "500 Internal Server Error";
}

std::string errorResponse(int code)
{
    std::string body = defaultErrorHTML(code);

    std::string response;

    response += "HTTP/1.1 " + statusMessage(code) + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + sizeToString(body.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;

    return response;
}