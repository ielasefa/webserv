#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <string.h>
#include <netinet/in.h>
#include <map>

typedef struct s_client {
	int fd;
	std::string req_buffer;
}	t_client;