*This project has been created as part of the 42 curriculum by iel-asef, aboukent and jaloulid*

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
./webserv file.conf
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

- https://en.wikipedia.org/wiki/Common_Gateway_Interface
- https://nginx.org/en/docs
- cppreference.com: C++ language and standard library reference

## AI Usage
Ai helped us to understand the concepts and in debugging, and it was not used to implement the server itself.