#include "webserv.hpp"

#include <iostream>
#include <string>

std::string handleRequest(const std::string& requestPath);

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

        std::string response = handleRequest(path);

        std::cout << "\n----- RESPONSE -----\n";
        std::cout << response << std::endl;
        std::cout << "--------------------\n";
    }

    return 0;
}
