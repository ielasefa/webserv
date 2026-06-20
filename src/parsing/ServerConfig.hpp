#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP
#include <string>
#include <vector>
#include <map>
#include "LocationConfig.hpp"

class ServerConfig
{
public:
    std::string host;

    int port;

    std::string server_name;

    std::string root;

    std::string index;

    size_t client_max_body_size;

    std::map<int, std::string> error_pages;

    std::vector<LocationConfig> locations;

    LocationConfig matchLocation(const std::string& path) const;


    ServerConfig();
};

#endif
