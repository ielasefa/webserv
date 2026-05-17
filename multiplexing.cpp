#include "webserv.hpp"

void	parsing_header(std::string req_buffer)
{
	t_header header;
	header.first_line = req_buffer.substr(0, req_buffer.find("\r"));
	header.method = header.first_line.substr(0, header.first_line.find(" "));
	header.other = header.first_line.substr(header.first_line.find(" ") + 1);
	header.path = header.other.substr(0, header.other.find(" "));
	header.version = header.other.substr(header.other.find(" ") + 1);

	std::cout << "===================\n" << "MTHD: " << header.method << std::endl
			<< "PATH: " << header.path << std::endl << "VRSN: " << header.version
			<< "\n===================" << std::endl;

}

void	add_epoll(int epfd, int fd)
{
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

void	multiplexing(int sfd)
{
	int epfd = epoll_create(1);
	add_epoll(epfd, sfd);
	struct epoll_event events[1024];

	while (1)
	{
		int e = epoll_wait(epfd, events, 2, -1);
		// std::cout << e;
		for (int i = 0; i < e; i++)
		{
			int fd = events[i].data.fd;
			std::map <int, t_client> clients;

			if (fd == sfd)
			{
				int cfd = accept(sfd, NULL, NULL);/////
				t_client cl;
				cl.fd = cfd;
				clients[cfd] = cl;
				add_epoll(epfd, cfd);
			}
			else
			{
				// handling request hna

				char buffer[1024] = "";
				int len = recv(fd, buffer, sizeof(buffer), 0);
				t_client &c = clients[fd];
				c.req_buffer += buffer;
				parsing_header(c.req_buffer);

				// std::cout << first_line << std::endl;
				// if (c.recv_buffer.find("\r\n\r\n") != std::string::npos)
				// 	std::cout << "---Yes---" << std::endl;
				// std::cout << len << std::endl;
				// std::cout << strlen(buffer) << std::endl;
				// std::cout << c.recv_buffer.size() << std::endl;
				// std::cout << "===================\n" << c.req_buffer << "===================" << std::endl;
				if (len <= 0)
				{
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					clients.erase(fd);
					close(fd);
				}
				else
				{
					// response mnb3d hna
					write(fd, "HTTP/1.1 200 OK\n\n<html><body>SAMAYKOM!!!</body></html>", 55);
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					close(fd);
				}
			}
		}
	}
}

int	init_socket()
{
	int sfd = socket(AF_INET, SOCK_STREAM, 0);

	int	opt = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // reuse of the port
	sockaddr_in	addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(3334); // hardcoded till we get it from config file
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		std::cout << "--shit--\n";

	listen(sfd, 4);
	return (sfd);
}

int main()
{
	int	sfd = init_socket();

	multiplexing(sfd);
}
