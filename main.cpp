#include "webserv.hpp"

#include <iostream>
#include <string>

std::string dispatchRequest(const Request& req);

int main()
{
    std::string path;

    initLocations();

    std::cout << "===== Webserv Test =====" << std::endl;

    while (true)
    {
        std::cout << "\nEnter path (or 'exit'): ";
        std::getline(std::cin, path);

        if (path == "exit")
            break;

        Request req;
        req.method = "GET";
        req.path = path;
        std::string response = dispatchRequest(req);

        std::cout << "\n----- RESPONSE -----\n";
        std::cout << response << std::endl;
        std::cout << "--------------------\n";
    }

    return 0;
}
