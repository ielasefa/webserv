/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaloulid <jaloulid@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 21:41:28 by iel-asef          #+#    #+#             */
/*   Updated: 2026/07/26 00:48:48 by jaloulid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

static std::string toStringSize(size_t value)
{
    std::ostringstream ss;
    ss << value;
    return ss.str();
}


std::string getMimeType(const std::string& path)
{
    std::string::size_type dot = path.rfind('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot);

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".html" || ext == ".htm")
        return "text/html";
    if (ext == ".css")
        return "text/css";
    if (ext == ".js")
        return "application/javascript";
    if (ext == ".json")
        return "application/json";
    if (ext == ".png")
        return "image/png";
    if (ext == ".jpg" || ext == ".jpeg")
        return "image/jpeg";
    if (ext == ".gif")
        return "image/gif";
    if (ext == ".svg")
        return "image/svg+xml";
    if (ext == ".ico")
        return "image/x-icon";
    if (ext == ".txt")
        return "text/plain";
    if (ext == ".pdf")
        return "application/pdf";
    if (ext == ".zip")
        return "application/zip";

    return "application/octet-stream";
}

std::string buildResponse(int code,
                          const std::string& body,
                          const std::string& mime,
                          const std::vector<std::string>& extraHeaders)
{
    std::string res;

    res += "HTTP/1.1 " + toStringSize(code) + " " + statusMessage(code) + "\r\n";

    if (!mime.empty())
        res += "Content-Type: " + mime + "\r\n";

    res += "Content-Length: " + toStringSize(body.size()) + "\r\n";
    res += "Connection: close\r\n";

    for (size_t i = 0; i < extraHeaders.size(); i++)
        res += extraHeaders[i] + "\r\n";

    res += "\r\n";
    res += body;

    return res;
}

std::string buildResponse(int code,
                          const std::string& body,
                          const std::string& mime)
{
    std::vector<std::string> emptyHeaders;
    return buildResponse(code, body, mime, emptyHeaders);
}
