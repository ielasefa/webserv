## To Learn

- HTTP protocol
- GET, POST, DELETE
- Static files
- CGI

- Basic config
- poll(), select()
- Socket
- Cookies

**HTTP** (Hyper Text Transfer Protocol)

A response-request protocol in the application layer of the OSI model. An HTTP server listens for an incoming client request on a socket bound to an address and a port. The address can either be a specific address configured on the host, or a wildcard to accept connection on all IP addresses configured on the host. By default the port is 80, but this is not required.

**Socket**

A socket is an endpoint of a connection and a way to speak to other programs on a computer or other computers on a network, and it is simply a file descriptor managed by the kernel.

socket() creates a socket and sets basic communication properties.

```cpp
<sys/socket.h>
int fd = socket(int DOMAIN, int TYPE, int PROTOCOL);
DOMAIN
// Defines what kind of communication space you’re in
// AF_INET = IPv4 | AF_INET6 = IPv6 | AF_UNIX = local com.
TYPE
// Defines how data is sent
// SOCK_STREAM = TCP | SOCK_DGRAM = UDP
PROTOCOL
// 0 = pick the default
```

bind() it attaches the socket to an address

```cpp
bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
sockfd
// the fd returned by socket()

addr
// the address to bind to
struct sockaddr_in addr;
addr.sin_family = AF_INET; // the communication domain
addr.sin_port = htons(8080); // the port
addr.sin_addr.s_addr = INADDR_ANY; // the ip address

addrlen
// the size of the address just sizeof(addr)
```

The second parameter of bind() is a struct which holds the socket address’s properties (address family, port, ip address), C doesn’t have inheritance so they created a generic struct for all the address families then a struct for each type (sockaddr_in, sockaddr_un…), we take a specific type (depends on the socket’s type) and we fill it. the variable sin_family is the address family (should match socket’s af), the variable sin_port is the port we’ll use.

htons()

Used because networks require a specific way of dealing with numbers, and prevents misinterpretation of numbers across different architectures. Most of the machines use little indian to store bytes in memory, but networks needs big indian which store bytes in reverse order.

listen() it makes the socket ready to accept incoming connections so the os start queuing them.

```cpp
listen(int sockfd, int backlog);
sockfd
// the fd returned by socket()
backlog
// max number of pending connections waiting in queue
```

accept() takes one waiting client from the listen() queue
and gives you a new socket to talk to it

```cpp
accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
sockfd
// the fd returned by socket()
// addr and addrlen we use only if we need to collect info about the client
// but usually we don't care so just fill them with NULL
```

**Multiplexing**

epoll is an API (mechanism) made of multiple syscalls, that together implement multiplexing, so it’s a whole system:

epoll_create() initialization, create new epoll instance inside kernel.

```cpp
#include <sys/epoll.h>
epoll_create(int size);
// returns a fd refers to the new epoll instance
// size is ignored
```

epoll_ctl() configuration, used to add, remove fds or modify what epoll is watching.

```cpp
epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
epfd
// the epoll instance returned by epoll_create()
op
// what to do
// EPOLL_CTL_ADD: start watching fd
// EPOLL_CTL_MOD: change what you watch for fd
// EPOLL_CTL_DEL: stop watching fd
fd
// the file descriptor managed (server or client socket)

event
// what you watch for
struct epoll event {
	uint32_t events;
	epoll_data_t data;
};
// events
// EPOLLIN: ready to read
// EPOLLOUT: ready to write
// data
// custom info, usually fd or pointer to a client object
```

epoll_wait() execution, wait for events.

```cpp
epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
epfd
// the epoll instance returned by epoll_create()
events
// output array where ready events are stored
maxevents
// size of the events array, and max events you can recive in a call
timeout
// how long to wait
// -1: wait forever
// 0: don't wait (non-blocking check)
// x: wait x milliseconds

RETURN // number of ready events
// so it be like: events[0], events[1]...
```