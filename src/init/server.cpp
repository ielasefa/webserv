#include "../../webserv.hpp"

bool create_socket(int& sfd, const std::vector<ServerConfig>& servers, int i, int j)
{
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
		return false;
	if (fcntl(sfd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(sfd);
		return false;
	}
	
	int opt = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		close(sfd);
		return false;
	}
	
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(servers[i].ports[j]);
	addr.sin_addr.s_addr = INADDR_ANY;	
	if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		close(sfd);
		return false;
	}

	if (listen(sfd, 128) < 0)
	{
		close(sfd);
		return false;
	}

	return true;
}

std::vector<int> init_socket(const std::vector<ServerConfig>& servers,
							std::map<int, std::vector<ServerConfig> >& socket_server)
{
	std::vector<int> sockets;
	std::map<int, int> port_socket;

	for (size_t i = 0; i < servers.size(); i++)
	{
		for (size_t j = 0; j < servers[i].ports.size(); j++)
		{
			int sfd;
	
			if (port_socket.find(servers[i].ports[j]) != port_socket.end())
				sfd = port_socket[servers[i].ports[j]];
			else
			{
				if (!create_socket(sfd, servers, i, j))
					return std::vector<int>();
				sockets.push_back(sfd);
				port_socket[servers[i].ports[j]] = sfd;
			}
			socket_server[sfd].push_back(servers[i]);
		}
	}
	return sockets;
}