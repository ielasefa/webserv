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


std::string handleDelete(const std::string& path)
{
    if (access(path.c_str(), F_OK) != 0)
        return errorResponse(404);

    if (!isFile(path))
        return errorResponse(403);
    if (isDirectory(path))
        return errorResponse(403);
    if (access(path.c_str(), W_OK) != 0)
        return errorResponse(403);

    if (remove(path.c_str()) != 0)
        return errorResponse(500);

    std::string response;
    response += "HTTP/1.1 204 No Content\r\n";
    response += "Content-Length: 0\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";

    return response;
}