#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <map>

class LocationConfig
{
public:
    std::string path;

    std::string root;

    std::string index;

    std::vector<std::string> allowed_methods;

    bool autoindex;

    std::string upload_store;

    int         redirect_code;
    std::string redirect;

    std::map<std::string, std::string> cgi_pass;

    LocationConfig();
};

#endif
