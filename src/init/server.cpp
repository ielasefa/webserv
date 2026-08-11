#include "../../webserv.hpp"

std::vector<int> init_socket(const std::vector<ServerConfig>& servers,
							std::map<int, std::vector<ServerConfig> >& socket_server)
{
	std::vector<int> sockets;
	std::map<int, int> port_socket;

	for (size_t i = 0; i < servers.size(); i++)
	{
		int sfd;

		if (port_socket.find(servers[i].port) != port_socket.end())
			sfd = port_socket[servers[i].port];
		else
		{
			sfd = socket(AF_INET, SOCK_STREAM, 0);
			fcntl(sfd, F_SETFL, O_NONBLOCK);

			int opt = 1;
			setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(servers[i].port);
			addr.sin_addr.s_addr = INADDR_ANY;

			if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
				std::cout << "--shit--" << std::endl;

			listen(sfd, 128);

			sockets.push_back(sfd);
			port_socket[servers[i].port] = sfd;
		}
		socket_server[sfd].push_back(servers[i]);
	}
	return sockets;
}