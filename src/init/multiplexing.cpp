#include "../../webserv.hpp"

t_client::t_client()
	: listen_socket(-1),
	  fd(-1),
	  timing(0),

	  fd_in(-1),
	  fd_out(-1),
	  pid(-1),
	  is_cgi(false),
	  cgi_timing(0),

	  is_header_parsed(false),
	  content_len(0),
	  is_chunked(false)
{}

ServerConfig& selecting_server(HttpRequest& req, std::vector<ServerConfig>& servers)
{
	for (size_t i = 0; i < servers.size(); i++)
	{
		if (servers[i].server_name == req.host)
			return (servers[i]);
	}
	return (servers[0]);
}

void	checking_timout(int epfd, std::map<int, t_client> &clients,
	std::map<int, std::vector<ServerConfig> >& socket_server)
{
	time_t now = time(NULL);
	std::map<int, t_client>::iterator it = clients.begin();
	while (it != clients.end())
	{
		if (it->second.is_cgi && now - it->second.cgi_timing > 20)
		{
			kill(it->second.pid, SIGKILL);
			
			int fd = it->second.fd;
			int fd_in = it->second.fd_in;
			int fd_out = it->second.fd_out;

			std::vector<ServerConfig> &servers = socket_server[it->second.listen_socket];
			ServerConfig &serv = selecting_server(it->second.req, servers);

			removing_client(epfd, fd_in, clients);
			removing_client(epfd, fd_out, clients);
			std::string response = errorResponse(504, "", &serv);
			write(fd, response.c_str(), response.size());
			removing_client(epfd, fd, clients);
			it = clients.begin();
		}
		else if (now - it->second.timing > 30)
		{
			int fd = it->first;
			it++;
			removing_client(epfd, fd, clients);
		}
		else
			it++;
	}
}

void	handling_cgiIn(int epfd, int fd, t_client &c, std::map<int, t_client> &clients)
{
	if (c.req.body.empty())
	{
		removing_client(epfd, fd, clients);
		return;
	}

	int len = write(c.fd_in, c.req.body.c_str(), c.req.body.size());
	if (len > 0)
	{
		c.req.body.erase(0, len);
		if (c.req.body.empty())
			removing_client(epfd, fd, clients);
	}
	else
	{
		std::cerr << "Error: failure in write()" << std::endl;
		removing_client(epfd, fd, clients);
	}
}

void	handling_cgiOut(int epfd, int fd, t_client &c, std::map<int, t_client> &clients)
{
	char buffer[1024] = "";
	int len = read(c.fd_out, buffer, sizeof(buffer));
	if (len > 0)
		c.cgi_output += std::string(buffer, len);
	else if (len == 0)
	{
		t_client &c_original = clients[c.fd];
		c_original.res_buffer = CGIHandler::wrapResponse(c.cgi_output);

		c_original.is_cgi = false;

		add_epoll(epfd, c_original.fd, 1);
		removing_client(epfd, fd, clients);
	}
	else
	{
		c.res_buffer = errorResponse(502, "", NULL);
		switching_toEPOLLOUT(epfd, c.fd);
	}
}

void	processing_completeReq(int epfd, int fd, t_client &c, std::map<int, t_client> &clients,
							std::map<int, std::vector<ServerConfig> > &socket_server)
{

	std::vector<ServerConfig> &servers = socket_server[c.listen_socket];
	ServerConfig& serv = selecting_server(c.req, servers);
	LocationConfig loc = serv.matchLocation(c.req.path);

	size_t body_pos = c.req_buffer.find("\r\n\r\n") + 4;
	if (c.is_chunked)
	{
		c.req.body = combining_chunks(c.req_buffer, body_pos);
	
		c.req.headers.erase("Transfer-Encoding");
		std::ostringstream oss;
		oss << c.req.body.size();
		c.req.headers["Content-Length"] = oss.str();
	}
	else
		c.req.body = c.req_buffer.substr(body_pos);

	std::string ext = "";
	size_t pos = c.req.path.find_last_of(".");
	if (pos != std::string::npos)
		ext = c.req.path.substr(pos);
	std::map<std::string, std::string>::iterator it = loc.cgi_pass.find(ext);
	if (it != loc.cgi_pass.end())
	{
		std::string scriptPath = buildPath(c.req.path, loc);
		CGIHandler cgi(c.req, scriptPath, loc.cgi_pass.at(ext));
		cgi.startCGI(c.fd_in, c.fd_out, c.pid);

		c.cgi_timing = time(NULL);
		c.is_cgi = 1;
		add_epoll(epfd, c.fd_in, 1);
		add_epoll(epfd, c.fd_out, 0);
		
		int c_original = fd;
		int tmp_fd_in = c.fd_in;
		int tmp_fd_out = c.fd_out;
		clients[tmp_fd_in] = c;
		clients[tmp_fd_out] = c;
		clients[tmp_fd_in].fd = c_original;
		clients[tmp_fd_out].fd = c_original;

		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	}
	else
	{
		c.res_buffer = dispatchRequest(c.req , serv);
		switching_toEPOLLOUT(epfd, fd);
	}
}

void	receiving_request(int epfd, int fd, t_client &c, std::map<int, t_client> &clients,
							std::map<int, std::vector<ServerConfig> > &socket_server)
{
	char buffer[1024] = "";
	int len = recv(fd, buffer, sizeof(buffer), 0);

	if (len < 0)
		return;
	if (len == 0)
	{
		if (c.is_chunked && !is_bodyComplete(c))
		{
			std::vector<ServerConfig> &servers = socket_server[c.listen_socket];
			ServerConfig &serv = selecting_server(c.req, servers);

			c.res_buffer = errorResponse(400, "", &serv);
			switching_toEPOLLOUT(epfd, fd);
			return;
		}
		removing_client(epfd, fd, clients);
		return;
	}
	else
	{
		c.req_buffer += std::string(buffer, len);
		c.timing = time(NULL);

		if (!c.is_header_parsed && c.req_buffer.find("\r\n\r\n") != std::string::npos)
		{
			c.req = parsing_header(c.req_buffer);
			c.is_header_parsed = true;
 
			if (c.req.headers.find("Content-Length") != c.req.headers.end())
				c.content_len = std::atoi(c.req.headers["Content-Length"].c_str());
			if (c.req.headers.find("Transfer-Encoding") != c.req.headers.end() && c.req.headers["Transfer-Encoding"] == "chunked")
				c.is_chunked = true;
		}

		if (c.is_header_parsed && is_bodyComplete(c))
			processing_completeReq(epfd, fd, c, clients, socket_server);
	}
}

void	handling_oldClients(int epfd, int fd, std::map<int, t_client> &clients,
								std::map<int, std::vector<ServerConfig> > &socket_server)
{
	t_client &c = clients[fd];

	if (c.is_cgi && fd == c.fd_out)
		handling_cgiOut(epfd, fd, c, clients);
	else if (c.is_cgi && fd == c.fd_in)
		handling_cgiIn(epfd, fd, c, clients);
	else
		receiving_request(epfd, fd, c, clients, socket_server);
}


void	handling_write(int epfd, int fd, std::map<int, t_client> &clients)
{
	t_client &c = clients[fd];

	int len = write(fd, c.res_buffer.c_str(), c.res_buffer.size());
	if (len > 0)
	{
		c.res_buffer.erase(0, len);
		if (c.res_buffer.empty())
		{
			if (c.req.headers["Connection"] == "close")
			{
				removing_client(epfd, fd, clients);
				return;
			}
		
			// keep-alive
			c.req_buffer.clear();
			c.req = HttpRequest();
			c.res_buffer.clear();
		
			c.is_header_parsed = false;
			c.content_len = 0;
			c.is_chunked = false;
			c.is_cgi = false;
			c.timing = time(NULL);
		
			switching_toEPOLLIN(epfd, fd);
		}
	}
	else if (len < 0)
		std::cerr << "Error: failure in write()" << std::endl;
}

void	handling_newClients(int epfd, int fd, std::map<int, t_client> &clients)
{
	int cfd = accept(fd, NULL, NULL);

	if (cfd < 0)
	{
		std::cerr << "Error: Failed to accept new client" << std::endl;
		return;
	}

	if (fcntl(cfd, F_SETFL, O_NONBLOCK) < 0)
	{
		std::cerr << "Error: Failure in fcntl()" << std::endl;
		close(cfd);
		return;
	}

	t_client cl;
	cl.listen_socket = fd;
	cl.fd = cfd;
	cl.timing = time(NULL);

	clients[cfd] = cl;
	add_epoll(epfd, cfd, 0);
}

void	multiplexing(std::vector<int> &sockets,
					std::map<int, std::vector<ServerConfig> >& socket_server)
{
	int epfd = epoll_create(1);
	for (size_t s = 0; s < sockets.size(); s++)
		add_epoll(epfd, sockets[s], 0);
	struct epoll_event events[1024];
	std::map <int, t_client> clients;
	
	while (1)
	{
		int e = epoll_wait(epfd, events, 1024, 1000);

		for (int i = 0; i < e; i++)
		{
			int fd = events[i].data.fd;

			if (socket_server.find(fd) != socket_server.end())
				handling_newClients(epfd, fd, clients);
			else if (clients[fd].is_cgi)
				handling_oldClients(epfd, fd, clients, socket_server);
			else if (events[i].events & EPOLLOUT)
				handling_write(epfd, fd, clients);
			else
				handling_oldClients(epfd, fd, clients, socket_server);
		}
		checking_timout(epfd, clients, socket_server);
	}
}
