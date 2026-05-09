#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <vector>

void send_request(int id) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(33344);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(s, (struct sockaddr *)&addr, sizeof(addr));

    std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    write(s, req.c_str(), req.size());

    char buf[4096] = {};
    read(s, buf, sizeof(buf));
    std::cout << "client " << id << " got: " << buf << "\n";
    close(s);
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++)
        threads.emplace_back(send_request, i);
    for (auto &t : threads)
        t.join();
}