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

typedef struct s_header
{
	std::string first_line;
	std::string method;
	std::string path;
	std::string version;
	std::string other;
}	t_header;