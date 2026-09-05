#include "webserv.hpp"

int main(int argc, char *argv[])
{
	signal(SIGPIPE, SIG_IGN);

	std::string config_path = "file.config";
	
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
		
		std::map<int, std::vector<ServerConfig> > socket_server;
		std::vector<int> sockets = init_socket(servers, socket_server);
		if (!sockets.size())
		{
			std::cerr << "Error: Failed to create socket(s)" << std::endl;
			return 1;
		}
		multiplexing(sockets, socket_server);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}
