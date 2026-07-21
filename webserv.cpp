#include "webserv.hpp"

const int   BACKLOG             = 128;
const int   POLL_TIMEOUT_MS     = -1;
const int   CGI_TIMEOUT_S       = 10;
const char *DEFAULT_CONFIG_PATH = "webserv.conf";
const int   READ_BUFFER_SIZE    = 4096;

int main(int argc, char *argv[])
{
	std::string config_path = DEFAULT_CONFIG_PATH;
	
	if (argc > 2)
	{
		std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
		return 1;
	}
	
	if (argc == 2)
		config_path = argv[1];
	
	try
	{
		ConfigParser parser(config_path);
		if (!parser.parse())
		{
			std::cerr << "Error: Failed to parse config file" << std::endl;
			return 1;
		}
		
		const std::vector<ServerConfig> &servers = parser.getServers();
		if (servers.empty())
		{
			std::cerr << "Error: No servers configured" << std::endl;
			return 1;
		}
		
		// std::cout << "Server started with " << servers.size() << " server(s)" << std::endl;
		int	sfd = init_socket(servers[0]);
		multiplexing(sfd , servers[0]);
		// std::cout << "samaykom" << std::endl;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}