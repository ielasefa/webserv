#include "../../webserv.hpp"

HttpRequest	parsing_header(std::string req_buffer)
{
	t_header header;
	HttpRequest req;

	header.first_line = req_buffer.substr(0, req_buffer.find("\r"));
	req.method = header.first_line.substr(0, header.first_line.find(" "));
	header.other = header.first_line.substr(header.first_line.find(" ") + 1);
	req.path = header.other.substr(0, header.other.find(" "));
	// header.version = header.other.substr(header.other.find(" ") + 1);

	if (req.method == "POST")
	{
		std::string tmp = req_buffer.substr(req_buffer.find("\r\n") + 2);
		std::string line;
		while (tmp.find("\r\n\r\n") != std::string::npos)
		{
			line = tmp.substr(0, tmp.find("\r\n"));
			// std::cout << "******************" << tmp << "******************" << std::endl;
			req.headers[line.substr(0, line.find(": "))] = line.substr(line.find(": ") + 2);
			// if (line.substr(line.find(": ") + 2).find("\r") != std::string::npos)
			// 	std::cout << "kaynaaaaaa";
			// std::cout << line << std::endl;
			// if (tmp.find("\r\n\r\n"))
			// 	std::cout << "wtf";
			tmp = tmp.substr(tmp.find("\r\n") + 2);
		}
		// std::cout << "***********\n"<< tmp << "\n***********\n";
	}
	// std::cout << "[" << req.headers["Sec-Fetch-Site"] << "]" << std::endl;
	// req.headers.insert({header.method, header.version});
	// std::cout << req.headers[header.method] << std::endl;
	// std::cout << std::endl;
	return (req);
}

void	add_epoll(int epfd, int fd)
{
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

void	multiplexing(int sfd ,const ServerConfig& serv)
{
	int epfd = epoll_create(1);
	add_epoll(epfd, sfd);
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
				add_epoll(epfd, cfd);
			}
			else
			{
				char buffer[1024] = "";
				int len = recv(fd, buffer, sizeof(buffer), 0);
				t_client &c = clients[fd];
				c.req_buffer += std::string(buffer, len);

				if (len <= 0)
				{
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					clients.erase(fd);
					close(fd);
				}
				else if (c.req_buffer.find("\r\n\r\n") != std::string::npos)
				{
					// std::cout << "port == " << serv.port << std::endl;

					// std::cout << c.req_buffer << std::endl;//
					// std::cout << "----------------\n" << c.req_buffer << "\n----------------\n";
					HttpRequest req = parsing_header(c.req_buffer);
					// std::cout << "smaykoooooooooooom" << std::endl;
					c.res_buffer = dispatchRequest(req , serv);
					// std::cout << "-------res_buffer---------\n" << c.res_buffer << "\n----------------\n";
					write(fd, c.res_buffer.c_str(), c.res_buffer.size());
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					clients.erase(fd);
					close(fd);
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