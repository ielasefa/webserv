#include "webserv.hpp"

static std::string toStringSize(size_t value)
{
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

std::string getMimeType(const std::string& path)
{
    // Minimal mapping for the evaluator; extend as needed.
    std::string::size_type dot = path.rfind('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot);

    if (ext == ".html" || ext == ".htm")
        return "text/html";
    if (ext == ".css")
        return "text/css";
    if (ext == ".js")
        return "application/javascript";
    if (ext == ".png")
        return "image/png";
    if (ext == ".jpg" || ext == ".jpeg")
        return "image/jpeg";
    if (ext == ".txt")
        return "text/plain";

    return "application/octet-stream";
}

std::string buildResponse(int code,
                          const std::string& body,
                          const std::string& mime,
                          const std::vector<std::string>& extraHeaders)
{
    std::string res;

    res += "HTTP/1.1 " + statusMessage(code) + "\r\n";

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
