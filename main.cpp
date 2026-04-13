#include "webserv.hpp"

int main()
{
	initLocations();

	std::cout << handleRequest("/images/") << std::endl;

	return 0;
}
