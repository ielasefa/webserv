#include "webserv.hpp"

std::vector<std::string> readDirectory(const std::string& path)
{
    std::vector<std::string> files;

    DIR* dir = opendir(path.c_str());
    if (!dir)
        return files;

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        files.push_back(name);
    }

    closedir(dir);
    return files;
}

std::string generateAutoIndex(const std::string& requestPath,
                              const std::vector<std::string>& files)
{
    std::string html;
    html += "<html><body>";
    html += "<h1>Index of " + requestPath + "</h1><ul>";

    for (size_t i = 0; i < files.size(); i++)
    {
        html += "<li><a href=\"" + requestPath + "/" + files[i] + "\">";
        html += files[i] + "</a></li>";
    }

    html += "</ul></body></html>";

    std::string res;
    res += "HTTP/1.1 200 OK\r\n";
    res += "Content-Type: text/html\r\n";
    res += "Content-Length: " + sizeToString(html.size()) + "\r\n";
    res += "Connection: close\r\n\r\n";
    res += html;

    return res;
}