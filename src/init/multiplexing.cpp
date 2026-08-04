#include "../../webserv.hpp"

HttpRequest	parsing_header(std::string req_buffer)
{
	t_header header;
	HttpRequest req;

	header.first_line = req_buffer.substr(0, req_buffer.find("\r"));
	req.method = header.first_line.substr(0, header.first_line.find(" "));
	header.other = header.first_line.substr(header.first_line.find(" ") + 1);
	req.path = header.other.substr(0, header.other.find(" "));

	if (req.method == "POST")
	{
		std::string tmp = req_buffer.substr(req_buffer.find("\r\n") + 2);
		std::string line;
		while (tmp.find("\r\n\r\n") != std::string::npos)
		{
			line = tmp.substr(0, tmp.find("\r\n"));
			req.headers[line.substr(0, line.find(": "))] = line.substr(line.find(": ") + 2);
			tmp = tmp.substr(tmp.find("\r\n") + 2);
		}
		// std::cout << tmp << std::endl;
		if (tmp.find("\r\n") != std::string::npos)
		{
			// std::cout << "in" << std::endl;
			req.body = tmp.substr(tmp.find("\r\n") + 2);
		}
		// std::cout << "["<< req.body << "]" << std::endl;
	}
	return (req);
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

void	multiplexing(int sfd ,const ServerConfig& serv)
{
	int epfd = epoll_create(1);
	add_epoll(epfd, sfd, 0);
	struct epoll_event events[1024];
	std::map <int, t_client> clients;

	while (1)
	{
		int e = epoll_wait(epfd, events, 2, -1);
		for (int i = 0; i < e; i++)
		{
			int fd = events[i].data.fd;

			if (fd == sfd)
			{
				int cfd = accept(sfd, NULL, NULL);//protection
				t_client cl;
				cl.fd = cfd;
				clients[cfd] = cl;
				add_epoll(epfd, cfd, 0);
			}
			else
			{
				t_client &c = clients[fd];
				// std::cout << "is_cgi=" << c.is_cgi << " fd=" << fd << std::endl;
				// std::cout << "fd=" << fd << " is_cgi=" << c.is_cgi << " c.fd_out=" << c.fd_out << std::endl;
				if (c.is_cgi && fd == c.fd_out)
				{
					// std::cout << "c.fd=" << c.fd << std::endl;
					char buffer[1024] = "";
					int len = read(c.fd_out, buffer, sizeof(buffer));
					c.cgi_output += std::string(buffer, len);
					// std::cout << "len=" << len << std::endl;
					// std::cout << c.cgi_output << std::endl;
					if (len == 0)
					{
						// std::cout << "wsl hna fd_out" << std::endl;
						t_client &c_original = clients[c.fd];
						// std::cout << "c.fd=" << c.fd << "\nc_original==" << c_original.fd << std::endl;
						c_original.res_buffer = CGIHandler::wrapResponse(c.cgi_output);
						write(c_original.fd, c_original.res_buffer.c_str(), c_original.res_buffer.size());
						epoll_ctl(epfd, EPOLL_CTL_DEL, c_original.fd, NULL);
						clients.erase(c_original.fd);
						close(c_original.fd);
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						clients.erase(fd);
						close(fd);
					}
				}
				else if (c.is_cgi && fd == c.fd_in)
				{
					// std::cout << "wsl hna fd_in" << std::endl;
					std::string body = c.req_buffer.substr(c.req_buffer.find("\r\n\r\n") + 4);
					write(c.fd_in, body.c_str(), body.size());
					epoll_ctl(epfd, EPOLL_CTL_DEL, c.fd_in, NULL);
					clients.erase(c.fd_in);
					close(c.fd_in);
				}
				else
				{
					char buffer[1024] = "";
					int len = recv(fd, buffer, sizeof(buffer), 0);
					c.req_buffer += std::string(buffer, len);
					if (len <= 0)
					{
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						clients.erase(fd);
						close(fd);
					}
					else if (c.req_buffer.find("\r\n\r\n") != std::string::npos)
					{
						HttpRequest req = parsing_header(c.req_buffer);
						LocationConfig loc = serv.matchLocation(req.path);
						// std::cout << loc.cgi_pass.first << std::endl;

						// for (std::map<std::string, std::string>::iterator it = loc.cgi_pass.begin(); it != loc.cgi_pass.end(); ++it)
						// 	std::cout << it->first << " " << it->second << std::endl;

						if (req.path.find(".py") != std::string::npos)
						{
							// handling_cgi(req, "f", "f", c, epfd);
							std::string scriptPath = buildPath(req.path, loc);
							std::cout << scriptPath << std::endl;
            				// if (!isFile(scriptPath))
                			// 	return errorResponse(404, "", &serv);
							CGIHandler cgi(req, scriptPath, loc.cgi_pass.at(".py"));
							cgi.startCGI(c.fd_in, c.fd_out, c.pid);
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
							c.res_buffer = dispatchRequest(req , serv);
							write(fd, c.res_buffer.c_str(), c.res_buffer.size());
							epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
							clients.erase(fd);
							close(fd);
	
						}
					}
				}
			}
		}
	}
}

int	init_socket(const ServerConfig& serv)
{
	int sfd = socket(AF_INET, SOCK_STREAM, 0);

	int	opt = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // reuse of the port
	sockaddr_in	addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serv.port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)//
		std::cout << "--shit--\n";

	listen(sfd, 4);
	return (sfd);
}