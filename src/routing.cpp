#include "webserv.hpp"

std::vector<Location> locations;

void initLocations()
{
    Location l1;
    l1.path = "/";
    l1.root = "www";
    l1.autoindex = false;

    Location l2;
    l2.path = "/images";
    l2.root = "www/images";
    l2.autoindex = true;

    locations.push_back(l1);
    locations.push_back(l2);
}

Location matchLocation(const std::string& requestPath)
{
    Location best = locations[0];

    for (size_t i = 0; i < locations.size(); i++)
    {
        const Location& loc = locations[i];

        if (requestPath == loc.path || requestPath.find(loc.path + "/") == 0)
        {
            if (loc.path.length() > best.path.length())
                best = loc;
        }
    }

    return best;
}

std::string buildPath(const std::string& requestPath, const Location& loc)
{
    std::string sub = requestPath.substr(loc.path.length());

    if (sub.empty() || sub == "/")
        return loc.root + "/index.html";

    return loc.root + sub;
}