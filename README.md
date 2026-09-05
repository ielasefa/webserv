*This project has been created as part of the 42 curriculum by iel-asef.*

# Description
This project is a custom HTTP server written in C++98. It parses a web server configuration file, listens on one or more ports, and serves static files, directory listings, uploads, redirects, error pages, CGI scripts, and basic HTTP methods such as GET, POST, HEAD, and DELETE.

The goal is to reproduce the core behavior of a web server with clear routing rules, configurable virtual hosts, and support for common development and testing workflows.

# Instructions
## Compilation
Build the project from the root of the repository:

```bash
make
```

Useful cleanup commands:

```bash
make clean
make fclean
make re
```

## Execution
Run the server with a configuration file:

```bash
./webserv server.conf
```

Other example configurations are available in the repository, such as `web.conf`.

## Configuration Notes
The configuration parser in this project uses `#` for comments. The provided configs define examples for:

- multiple listen ports
- virtual hosts
- CGI execution
- uploads and deletions
- redirects
- autoindex

# Resources
Reference material used while developing and documenting this project:

- MDN Web Docs: HTTP overview and request/response basics
- RFC 9110: HTTP Semantics
- RFC 9112: HTTP/1.1
- RFC 3875: CGI 1.1
- cppreference.com: C++ language and standard library reference

## AI Usage
AI was used to help draft and structure this README, summarize the build and execution commands from the Makefile and configuration files, and verify that the documentation matches the current repository layout. AI was not used to implement the server itself.