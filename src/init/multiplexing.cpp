#include "../../webserv.hpp"

HttpRequest parsing_header(std::string req_buffer)
{
    HttpRequest req;

    size_t lineEnd = req_buffer.find("\r\n");

    if (lineEnd == std::string::npos)
        return req;

    std::string line = req_buffer.substr(0, lineEnd);

    size_t firstSpace = line.find(' ');

    if (firstSpace == std::string::npos)
        return req;

    size_t secondSpace = line.find(' ', firstSpace + 1);

    if (secondSpace == std::string::npos)
    {
        req.method = line.substr(0, firstSpace);
        req.path = line.substr(firstSpace + 1);
        req.version = "";
        return req;
    }

    // Request line must contain exactly 3 tokens
    if (line.find(' ', secondSpace + 1) != std::string::npos)
        return req;

    req.method = line.substr(0, firstSpace);

    std::string target =
        line.substr(firstSpace + 1,
                    secondSpace - firstSpace - 1);

    req.version = line.substr(secondSpace + 1);

    size_t queryPos = target.find('?');

    if (queryPos != std::string::npos)
    {
        req.path = target.substr(0, queryPos);
        req.query_string = target.substr(queryPos + 1);
    }
    else
        req.path = target;

    std::string tmp = req_buffer.substr(lineEnd + 2);

    while (tmp.find("\r\n") != std::string::npos)
    {
        line = tmp.substr(0, tmp.find("\r\n"));

        if (line.empty())
            break;

        size_t pos = line.find(": ");

        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 2);

            req.headers[key] = value;
        }

        tmp = tmp.substr(tmp.find("\r\n") + 2);
    }

    if (req.headers.find("Host") != req.headers.end())
    {
        tmp = req.headers["Host"];

        size_t colon = tmp.find(':');

        if (colon != std::string::npos)
            req.host = tmp.substr(0, colon);
        else
            req.host = tmp;
    }

    return req;
}
void	add_epoll(int epfd, int fd, bool type)
{
	struct epoll_event ev;
	if (type)
		ev.events = EPOLLOUT;
	else
		ev.events = EPOLLIN;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}
void	handling_cgi(HttpRequest &request, std::string script_path,
					std::string cgi_executable, t_client &c, int epfd)
{
	CGIHandler cgi(request, script_path, cgi_executable);
	cgi.startCGI(c.fd_in, c.fd_out, c.pid);
	c.is_cgi = 1;
	add_epoll(epfd, c.fd_in, 1);
	add_epoll(epfd, c.fd_out, 0);
}

ServerConfig& selecting_server(HttpRequest& req, std::vector<ServerConfig>& servers)
{
	for (size_t i = 0; i < servers.size(); i++)
	{
		if (servers[i].server_name == req.host)
			return (servers[i]);
	}
	return (servers[0]);
}

bool is_bodyComplete(t_client &c)
{
	size_t header_end = c.req_buffer.find("\r\n\r\n");
	if (header_end == std::string::npos)
		return false;

	size_t body_start = header_end + 4;

	if (c.is_chunked)
		return c.req_buffer.find("0\r\n\r\n", body_start) != std::string::npos;

	if (c.content_len == 0)
		return true;

	size_t body_received = c.req_buffer.size() - body_start;
	return body_received >= c.content_len;
}

void	removing_client(int epfd, int fd, std::map<int, t_client> &clients)
{
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	clients.erase(fd);
	close(fd);
}

void	checking_timout(int epfd, std::map<int, t_client> &clients,
	std::map<int, std::vector<ServerConfig> >& socket_server)
{
	time_t now = time(NULL);
	std::map<int, t_client>::iterator it = clients.begin();
	while (it != clients.end())
	{
		if (it->second.is_cgi && now - it->second.cgi_timing > 5)
		{
			kill(it->second.pid, SIGKILL);

			int fd = it->first;
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

			std::cout << "cgi client removed\n";
		}
		else if (now - it->second.timing > 5)
		{
			int fd = it->first;
			it++;
			removing_client(epfd, fd, clients);
			std::cout << "client removed\n";
		}
		else
			it++;
	}
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
		int e = epoll_wait(epfd, events, 2, 1000);
		checking_timout(epfd, clients, socket_server);

		for (int i = 0; i < e; i++)
		{
			int fd = events[i].data.fd;

			if (socket_server.find(fd) != socket_server.end())
			{
				int cfd = accept(fd, NULL, NULL);
				if (cfd < 0)
				{
					std::cerr << "Error: Failed to accept new client" << std::endl;
					continue;
				}
				if (fcntl(cfd, F_SETFL, O_NONBLOCK) < 0)
				{
					std::cerr << "Error: Failure in fcntl()" << std::endl;
					close(cfd);
					continue;
				}
				t_client cl;
				cl.listen_socket = fd;
				cl.fd = cfd;
				cl.timing = time(NULL);
				// std::cout << cl.timing << "time accpeting" << std::endl;
				clients[cfd] = cl;
				add_epoll(epfd, cfd, 0);
			}
			else
			{
				t_client &c = clients[fd];

				if (c.is_cgi && fd == c.fd_out)
				{
					char buffer[1024] = "";
					int len = read(c.fd_out, buffer, sizeof(buffer));
					c.cgi_output += std::string(buffer, len);
					if (len == 0)
					{
						t_client &c_original = clients[c.fd];
						c_original.res_buffer = CGIHandler::wrapResponse(c.cgi_output);
						write(c_original.fd, c_original.res_buffer.c_str(), c_original.res_buffer.size());
						removing_client(epfd, c_original.fd, clients);
						removing_client(epfd, fd, clients);
					}
				}
				else if (c.is_cgi && fd == c.fd_in)
				{
					std::string body = c.req_buffer.substr(c.req_buffer.find("\r\n\r\n") + 4);
					write(c.fd_in, body.c_str(), body.size());
					removing_client(epfd, c.fd_in, clients);
				}
				else
				{
					char buffer[1024] = "";
					int len = recv(fd, buffer, sizeof(buffer), 0);

					if (len <= 0)
						removing_client(epfd, fd, clients);
					else
					{
						c.req_buffer += std::string(buffer, len);
						c.timing = time(NULL);
						// std::cout << c.timing << "time receiving" << std::endl;
						if (!c.is_header_parsed && c.req_buffer.find("\r\n\r\n") != std::string::npos)
						{
							c.req = parsing_header(c.req_buffer);
							c.is_header_parsed = true;

							if (c.req.headers.find("Content-Length") != c.req.headers.end())
								c.content_len = std::atoi(c.req.headers["Content-Length"].c_str());
							if (c.req.headers.find("Transfer-Encoding") != c.req.headers.end() && c.req.headers["Transfer-Encoding"] == "chunked")
								c.is_chunked = true;
						}
						// HttpRequest req = parsing_header(c.req_buffer);
						if (c.is_header_parsed && is_bodyComplete(c))
						{
							size_t body_pos = c.req_buffer.find("\r\n\r\n") + 4;
							if (body_pos != std::string::npos)
								c.req.body = c.req_buffer.substr(body_pos);

							std::vector<ServerConfig> &servers = socket_server[c.listen_socket];
							ServerConfig& serv = selecting_server(c.req, servers);
							LocationConfig loc = serv.matchLocation(c.req.path);

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
								// std::cout << "clients[fd].fd=" << clients[fd].fd << std::endl;
								// std::cout << "wsl hna mor startCGI" << std::endl;
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
	
								// struct epoll_event ev;
								// ev.events = EPOLLOUT;
								// ev.data.fd = fd;
								// epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
								epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
								// std::cout << "clients[c.fd_out].fd=" << clients[c.fd_out].fd << std::endl;
							}
							else
							{
								// std::cout << "mcha\n";
								c.res_buffer = dispatchRequest(c.req , serv);
								write(fd, c.res_buffer.c_str(), c.res_buffer.size());
								removing_client(epfd, fd, clients);
							}
						}
					}
				}
			}
		}
	}
}