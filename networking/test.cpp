#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <netinet/in.h>

int main()
{
	int sfd = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in	addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(33344);
	addr.sin_addr.s_addr = INADDR_ANY;
	bind(sfd, (struct sockaddr *)&addr, sizeof(addr));

	listen(sfd, 4);

	while (1)
	{
		int cfd = accept(sfd, NULL, NULL);

		char buffer[1024] = "";
		read(cfd, buffer, sizeof(buffer) - 1);
		write(cfd, "HTTP/1.1 200 OK\n\n<html><body>SAMAYKOM!!!</body></html>", 55);

		close(cfd);
	}
}