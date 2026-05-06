#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <netinet/in.h>

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
		std::cout << e;
		for (int i = 0; i < e; i++)
		{
			int fd = events[i].data.fd;
			if (fd == sfd)
			{
				int cfd = accept(sfd, NULL, NULL);
				add_epoll(epfd, cfd);
			}
			else
			{
				char buffer[1024] = "";
				int len = read(fd, buffer, sizeof(buffer) - 1);
				std::cout << len << std::endl;
				std::cout << buffer << std::endl;
				if (len <= 0)
				{
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					close(fd);
				}
				else
				{
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

	sockaddr_in	addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(3334);
	addr.sin_addr.s_addr = INADDR_ANY;
	bind(sfd, (struct sockaddr *)&addr, sizeof(addr));

	listen(sfd, 4);
	return (sfd);
}

int main()
{
	int	sfd = init_socket();

	multiplexing(sfd);
}
