#include "../../webserv.hpp"

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

void	removing_client(int epfd, int fd, std::map<int, t_client> &clients)
{
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	clients.erase(fd);
	close(fd);
}

void	switching_toEPOLLIN(int epfd, int fd)
{
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

void	switching_toEPOLLOUT(int epfd, int fd)
{
	struct epoll_event ev;
	ev.events = EPOLLOUT;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}