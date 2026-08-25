#include "../../webserv.hpp"

/*
 * ============================================================
 * REQUEST PARSING
 * ============================================================
 */

HttpRequest parsing_header(std::string req_buffer)
{
    HttpRequest req;

    size_t lineEnd = req_buffer.find("\r\n");

    if (lineEnd == std::string::npos)
        return req;

    std::string line = req_buffer.substr(0, lineEnd);

    size_t firstSpace = line.find(' ');

    if (firstSpace == std::string::npos)
        return req;

    size_t secondSpace = line.find(' ', firstSpace + 1);

    if (secondSpace == std::string::npos)
    {
        req.method = line.substr(0, firstSpace);
        req.path = line.substr(firstSpace + 1);
        req.version = "";
        return req;
    }

    /*
     * Request line must contain exactly 3 tokens.
     */
    if (line.find(' ', secondSpace + 1) != std::string::npos)
        return req;

    req.method = line.substr(0, firstSpace);

    std::string target =
        line.substr(
            firstSpace + 1,
            secondSpace - firstSpace - 1
        );

    req.version = line.substr(secondSpace + 1);

    size_t queryPos = target.find('?');

    if (queryPos != std::string::npos)
    {
        req.path =
            target.substr(0, queryPos);

        req.query_string =
            target.substr(queryPos + 1);
    }
    else
    {
        req.path = target;
    }

    std::string tmp =
        req_buffer.substr(lineEnd + 2);

    while (tmp.find("\r\n") != std::string::npos)
    {
        line =
            tmp.substr(0, tmp.find("\r\n"));

        if (line.empty())
            break;

        size_t pos = line.find(": ");

        if (pos != std::string::npos)
        {
            std::string key =
                line.substr(0, pos);

            std::string value =
                line.substr(pos + 2);

            req.headers[key] = value;
        }

        tmp =
            tmp.substr(
                tmp.find("\r\n") + 2
            );
    }

    if (req.headers.find("Host") !=
        req.headers.end())
    {
        tmp = req.headers["Host"];

        size_t colon = tmp.find(':');

        if (colon != std::string::npos)
            req.host = tmp.substr(0, colon);
        else
            req.host = tmp;
    }

    return req;
}


/*
 * ============================================================
 * EPOLL
 * ============================================================
 */

void add_epoll(int epfd, int fd, bool type)
{
    struct epoll_event ev;

    if (type)
        ev.events = EPOLLOUT;
    else
        ev.events = EPOLLIN;

    ev.data.fd = fd;

    epoll_ctl(
        epfd,
        EPOLL_CTL_ADD,
        fd,
        &ev
    );
}


/*
 * ============================================================
 * CGI
 * ============================================================
 */

void handling_cgi(HttpRequest &request,
                  std::string script_path,
                  std::string cgi_executable,
                  t_client &c,
                  int epfd)
{
    CGIHandler cgi(
        request,
        script_path,
        cgi_executable
    );

    cgi.startCGI(
        c.fd_in,
        c.fd_out,
        c.pid
    );

    c.is_cgi = 1;

    add_epoll(
        epfd,
        c.fd_in,
        1
    );

    add_epoll(
        epfd,
        c.fd_out,
        0
    );
}


/*
 * ============================================================
 * SERVER SELECTION
 * ============================================================
 */

ServerConfig& selecting_server(
    HttpRequest& req,
    std::vector<ServerConfig>& servers)
{
    for (size_t i = 0;
         i < servers.size();
         ++i)
    {
        if (servers[i].server_name == req.host)
            return servers[i];
    }

    return servers[0];
}


/*
 * ============================================================
 * CHUNKED BODY
 * ============================================================
 */

enum e_chunk_status
{
    CHUNK_INVALID = -1,
    CHUNK_INCOMPLETE = 0,
    CHUNK_COMPLETE = 1
};


static int chunkHexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return -1;
}


/*
 * Decode:
 *
 * 4\r\n
 * Wiki\r\n
 * 5\r\n
 * pedia\r\n
 * 0\r\n
 * \r\n
 *
 * =>
 *
 * Wikipedia
 *
 *
 * Also supports:
 *
 * A;foo=bar\r\n
 *
 * and trailers:
 *
 * 0\r\n
 * X-Test: hello\r\n
 * \r\n
 */
static int decodeChunkedBody(
    const std::string& body,
    std::string& decoded)
{
    decoded.clear();

    size_t pos = 0;

    while (true)
    {
        /*
         * Need complete chunk-size line.
         */
        size_t lineEnd =
            body.find("\r\n", pos);

        if (lineEnd == std::string::npos)
            return CHUNK_INCOMPLETE;

        std::string sizeLine =
            body.substr(
                pos,
                lineEnd - pos
            );

        /*
         * Ignore chunk extensions.
         *
         * A;foo=bar
         *
         * =>
         *
         * A
         */
        size_t semicolon =
            sizeLine.find(';');

        if (semicolon != std::string::npos)
        {
            sizeLine =
                sizeLine.substr(
                    0,
                    semicolon
                );
        }

        if (sizeLine.empty())
            return CHUNK_INVALID;

        size_t chunkSize = 0;

        for (size_t i = 0;
             i < sizeLine.size();
             ++i)
        {
            int value =
                chunkHexValue(sizeLine[i]);

            if (value == -1)
                return CHUNK_INVALID;

            chunkSize =
                chunkSize * 16 +
                static_cast<size_t>(value);
        }

        /*
         * Move after:
         *
         * <chunk-size>\r\n
         */
        pos = lineEnd + 2;

        /*
         * Final chunk.
         */
        if (chunkSize == 0)
        {
            /*
             * Normal:
             *
             * 0\r\n
             * \r\n
             */
            if (body.size() >= pos + 2 &&
                body.compare(pos, 2, "\r\n") == 0)
            {
                return CHUNK_COMPLETE;
            }

            /*
             * With trailers:
             *
             * 0\r\n
             * X-Test: hello\r\n
             * Foo: bar\r\n
             * \r\n
             */
            size_t trailerEnd =
                body.find(
                    "\r\n\r\n",
                    pos
                );

            if (trailerEnd ==
                std::string::npos)
            {
                return CHUNK_INCOMPLETE;
            }

            return CHUNK_COMPLETE;
        }

        /*
         * Need:
         *
         * chunk data
         * +
         * trailing \r\n
         */
        if (body.size() <
            pos + chunkSize + 2)
        {
            return CHUNK_INCOMPLETE;
        }

        decoded.append(
            body,
            pos,
            chunkSize
        );

        pos += chunkSize;

        /*
         * Every chunk data must end in CRLF.
         */
        if (body.compare(
                pos,
                2,
                "\r\n") != 0)
        {
            return CHUNK_INVALID;
        }

        pos += 2;
    }
}


/*
 * ============================================================
 * BODY COMPLETE
 * ============================================================
 */

bool is_bodyComplete(t_client &c)
{
    size_t header_end =
        c.req_buffer.find("\r\n\r\n");

    if (header_end == std::string::npos)
        return false;

    size_t body_start =
        header_end + 4;

    /*
     * Chunked body.
     */
    if (c.is_chunked)
    {
        std::string rawBody =
            c.req_buffer.substr(
                body_start
            );

        std::string decoded;

        int status =
            decodeChunkedBody(
                rawBody,
                decoded
            );

        /*
         * INVALID returns true too,
         * so multiplexing() can send 400.
         */
        if (status == CHUNK_INVALID)
            return true;

        return status == CHUNK_COMPLETE;
    }

    /*
     * No body.
     */
    if (c.content_len == 0)
        return true;

    size_t body_received =
        c.req_buffer.size() -
        body_start;

    return body_received >=
        c.content_len;
}


/*
 * ============================================================
 * REMOVE CLIENT
 * ============================================================
 */

void removing_client(
    int epfd,
    int fd,
    std::map<int, t_client> &clients)
{
    epoll_ctl(
        epfd,
        EPOLL_CTL_DEL,
        fd,
        NULL
    );

    clients.erase(fd);

    close(fd);
}


/*
 * ============================================================
 * TIMEOUT
 * ============================================================
 */

void checking_timout(
    int epfd,
    std::map<int, t_client> &clients,
    std::map<int,
    std::vector<ServerConfig> >& socket_server)
{
    time_t now = time(NULL);

    std::map<int, t_client>::iterator it =
        clients.begin();

    while (it != clients.end())
    {
        if (it->second.is_cgi &&
            now - it->second.cgi_timing > 5)
        {
            kill(
                it->second.pid,
                SIGKILL
            );

            int fd =
                it->first;

            int fd_in =
                it->second.fd_in;

            int fd_out =
                it->second.fd_out;

            std::vector<ServerConfig> &servers =
                socket_server[
                    it->second.listen_socket
                ];

            ServerConfig &serv =
                selecting_server(
                    it->second.req,
                    servers
                );

            removing_client(
                epfd,
                fd_in,
                clients
            );

            removing_client(
                epfd,
                fd_out,
                clients
            );

            std::string response =
                errorResponse(
                    504,
                    "",
                    &serv
                );

            write(
                fd,
                response.c_str(),
                response.size()
            );

            removing_client(
                epfd,
                fd,
                clients
            );

            it = clients.begin();

            std::cout
                << "cgi client removed\n";
        }
        else if (now - it->second.timing > 5)
        {
            int fd = it->first;

            ++it;

            removing_client(
                epfd,
                fd,
                clients
            );

            std::cout
                << "client removed\n";
        }
        else
        {
            ++it;
        }
    }
}


/*
 * ============================================================
 * MULTIPLEXING
 * ============================================================
 */

void multiplexing(
    std::vector<int> &sockets,
    std::map<int,
    std::vector<ServerConfig> >& socket_server)
{
    int epfd =
        epoll_create(1);

    for (size_t s = 0;
         s < sockets.size();
         ++s)
    {
        add_epoll(
            epfd,
            sockets[s],
            0
        );
    }

    struct epoll_event events[1024];

    std::map<int, t_client> clients;


    while (1)
    {
        int e =
            epoll_wait(
                epfd,
                events,
                2,
                1000
            );

        checking_timout(
            epfd,
            clients,
            socket_server
        );


        for (int i = 0;
             i < e;
             ++i)
        {
            int fd =
                events[i].data.fd;


            /*
             * ====================================================
             * SERVER SOCKET => ACCEPT
             * ====================================================
             */
            if (socket_server.find(fd) !=
                socket_server.end())
            {
                int cfd =
                    accept(
                        fd,
                        NULL,
                        NULL
                    );

                if (cfd < 0)
                {
                    std::cerr
                        << "Error: Failed to accept new client"
                        << std::endl;

                    continue;
                }

                if (fcntl(
                        cfd,
                        F_SETFL,
                        O_NONBLOCK) < 0)
                {
                    std::cerr
                        << "Error: Failure in fcntl()"
                        << std::endl;

                    close(cfd);

                    continue;
                }

                t_client cl;

                cl.listen_socket = fd;
                cl.fd = cfd;
                cl.timing = time(NULL);

                clients[cfd] = cl;

                add_epoll(
                    epfd,
                    cfd,
                    0
                );

                continue;
            }


            /*
             * Ignore stale epoll event.
             */
            if (clients.find(fd) ==
                clients.end())
            {
                continue;
            }


            t_client &c =
                clients[fd];


            /*
             * ====================================================
             * CGI STDOUT
             * ====================================================
             */
            if (c.is_cgi &&
                fd == c.fd_out)
            {
                char buffer[1024];

                int len =
                    read(
                        c.fd_out,
                        buffer,
                        sizeof(buffer)
                    );

                if (len > 0)
                {
                    c.cgi_output.append(
                        buffer,
                        len
                    );
                }
                else if (len == 0)
                {
                    /*
                     * CGI finished.
                     */
                    if (clients.find(c.fd) ==
                        clients.end())
                    {
                        removing_client(
                            epfd,
                            fd,
                            clients
                        );

                        continue;
                    }

                    int original_fd =
                        c.fd;

                    std::string output =
                        c.cgi_output;

                    t_client &c_original =
                        clients[original_fd];

                    c_original.res_buffer =
                        CGIHandler::wrapResponse(
                            output
                        );

                    write(
                        c_original.fd,
                        c_original.res_buffer.c_str(),
                        c_original.res_buffer.size()
                    );

                    removing_client(
                        epfd,
                        original_fd,
                        clients
                    );

                    removing_client(
                        epfd,
                        fd,
                        clients
                    );
                }
                else
                {
                    /*
                     * Non-blocking read may return EAGAIN.
                     */
                    if (errno == EAGAIN ||
                        errno == EWOULDBLOCK)
                    {
                        continue;
                    }

                    removing_client(
                        epfd,
                        fd,
                        clients
                    );
                }
            }


            /*
             * ====================================================
             * CGI STDIN
             * ====================================================
             */
            else if (c.is_cgi &&
                     fd == c.fd_in)
            {
                /*
                 * IMPORTANT:
                 *
                 * DO NOT read body again from req_buffer.
                 *
                 * c.req.body is already decoded/
                 * unchunked.
                 */

                while (!c.req.body.empty())
                {
                    ssize_t written =
                        write(
                            c.fd_in,
                            c.req.body.c_str(),
                            c.req.body.size()
                        );

                    if (written > 0)
                    {
                        /*
                         * Remove bytes already sent.
                         *
                         * This handles large bodies and
                         * partial pipe writes.
                         */
                        c.req.body.erase(
                            0,
                            static_cast<size_t>(written)
                        );

                        continue;
                    }

                    if (written < 0 &&
                        (errno == EAGAIN ||
                         errno == EWOULDBLOCK))
                    {
                        /*
                         * Keep fd_in in epoll.
                         * Next EPOLLOUT continues writing.
                         */
                        break;
                    }

                    /*
                     * Real write error.
                     */
                    removing_client(
                        epfd,
                        fd,
                        clients
                    );

                    break;
                }

                /*
                 * Everything written to CGI.
                 *
                 * Close stdin so CGI receives EOF.
                 */
                if (clients.find(fd) !=
                        clients.end() &&
                    clients[fd].req.body.empty())
                {
                    removing_client(
                        epfd,
                        fd,
                        clients
                    );
                }
            }


            /*
             * ====================================================
             * NORMAL CLIENT SOCKET
             * ====================================================
             */
            else
            {
                char buffer[1024];

                int len =
                    recv(
                        fd,
                        buffer,
                        sizeof(buffer),
                        0
                    );


                /*
                 * =================================================
                 * PEER CLOSED CONNECTION
                 * =================================================
                 */
                if (len == 0)
                {
                    /*
                     * If a chunked request started but
                     * never completed:
                     *
                     * => 400 Bad Request.
                     */
                    if (c.is_header_parsed &&
                        c.is_chunked)
                    {
                        size_t header_end =
                            c.req_buffer.find(
                                "\r\n\r\n"
                            );

                        if (header_end !=
                            std::string::npos)
                        {
                            std::string rawBody =
                                c.req_buffer.substr(
                                    header_end + 4
                                );

                            std::string decodedBody;

                            int status =
                                decodeChunkedBody(
                                    rawBody,
                                    decodedBody
                                );

                            if (status !=
                                CHUNK_COMPLETE)
                            {
                                std::vector<ServerConfig> &servers =
                                    socket_server[
                                        c.listen_socket
                                    ];

                                ServerConfig &serv =
                                    selecting_server(
                                        c.req,
                                        servers
                                    );

                                std::string response =
                                    errorResponse(
                                        400,
                                        "",
                                        &serv
                                    );

                                write(
                                    fd,
                                    response.c_str(),
                                    response.size()
                                );
                            }
                        }
                    }

                    removing_client(
                        epfd,
                        fd,
                        clients
                    );

                    continue;
                }


                /*
                 * recv error.
                 */
                if (len < 0)
                {
                    if (errno == EAGAIN ||
                        errno == EWOULDBLOCK)
                    {
                        continue;
                    }

                    removing_client(
                        epfd,
                        fd,
                        clients
                    );

                    continue;
                }


                /*
                 * =================================================
                 * DATA RECEIVED
                 * =================================================
                 */

                c.req_buffer.append(
                    buffer,
                    len
                );

                c.timing = time(NULL);


                /*
                 * =================================================
                 * PARSE HEADERS ONCE
                 * =================================================
                 */
                if (!c.is_header_parsed &&
                    c.req_buffer.find(
                        "\r\n\r\n") !=
                        std::string::npos)
                {
                    c.req =
                        parsing_header(
                            c.req_buffer
                        );

                    c.is_header_parsed =
                        true;


                    /*
                     * Content-Length.
                     */
                    if (c.req.headers.find(
                            "Content-Length") !=
                        c.req.headers.end())
                    {
                        c.content_len =
                            std::atoi(
                                c.req.headers[
                                    "Content-Length"
                                ].c_str()
                            );
                    }


                    /*
                     * Transfer-Encoding: chunked.
                     */
                    if (c.req.headers.find(
                            "Transfer-Encoding") !=
                            c.req.headers.end() &&
                        c.req.headers[
                            "Transfer-Encoding"
                        ] == "chunked")
                    {
                        c.is_chunked = true;
                    }
                }


                /*
                 * =================================================
                 * COMPLETE REQUEST BODY
                 * =================================================
                 */
                if (c.is_header_parsed &&
                    is_bodyComplete(c))
                {
                    size_t header_end =
                        c.req_buffer.find(
                            "\r\n\r\n"
                        );

                    size_t body_pos =
                        header_end + 4;

                    std::string rawBody =
                        c.req_buffer.substr(
                            body_pos
                        );


                    /*
                     * =================================================
                     * UNCHUNK BODY BEFORE CGI / POST
                     * =================================================
                     */
                    if (c.is_chunked)
                    {
                        std::string decodedBody;

                        int chunkStatus =
                            decodeChunkedBody(
                                rawBody,
                                decodedBody
                            );


                        /*
                         * Malformed chunk syntax.
                         */
                        if (chunkStatus ==
                            CHUNK_INVALID)
                        {
                            std::vector<ServerConfig> &servers =
                                socket_server[
                                    c.listen_socket
                                ];

                            ServerConfig &serv =
                                selecting_server(
                                    c.req,
                                    servers
                                );

                            std::string response =
                                errorResponse(
                                    400,
                                    "",
                                    &serv
                                );

                            write(
                                fd,
                                response.c_str(),
                                response.size()
                            );

                            removing_client(
                                epfd,
                                fd,
                                clients
                            );

                            continue;
                        }


                        /*
                         * Should normally not happen because
                         * is_bodyComplete() already checked.
                         */
                        if (chunkStatus !=
                            CHUNK_COMPLETE)
                        {
                            continue;
                        }


                        /*
                         * Clean assembled body.
                         */
                        c.req.body =
                            decodedBody;


                        /*
                         * Transport decoding is done.
                         *
                         * Downstream POST/CGI should NOT try
                         * to unchunk it a second time.
                         */
                        c.req.headers.erase(
                            "Transfer-Encoding"
                        );


                        /*
                         * Set Content-Length to decoded body
                         * size for CGI / downstream handlers.
                         */
                        std::ostringstream bodyLength;

                        bodyLength
                            << decodedBody.size();

                        c.req.headers[
                            "Content-Length"
                        ] = bodyLength.str();

                        c.content_len =
                            decodedBody.size();
                    }
                    else
                    {
                        c.req.body =
                            rawBody;
                    }


                    /*
                     * =================================================
                     * SELECT SERVER / LOCATION
                     * =================================================
                     */

                    std::vector<ServerConfig> &servers =
                        socket_server[
                            c.listen_socket
                        ];

                    ServerConfig &serv =
                        selecting_server(
                            c.req,
                            servers
                        );

                    LocationConfig loc =
                        serv.matchLocation(
                            c.req.path
                        );


                    /*
                     * =================================================
                     * CGI DETECTION
                     * =================================================
                     */

                    std::string ext = "";

                    size_t pos =
                        c.req.path.find_last_of(".");

                    if (pos != std::string::npos)
                    {
                        ext =
                            c.req.path.substr(pos);
                    }

                    std::map<
                        std::string,
                        std::string
                    >::iterator it =
                        loc.cgi_pass.find(ext);


                    /*
                     * =================================================
                     * CGI REQUEST
                     * =================================================
                     */
                    if (it != loc.cgi_pass.end())
                    {
                        std::string scriptPath =
                            buildPath(
                                c.req.path,
                                loc
                            );

                        CGIHandler cgi(
                            c.req,
                            scriptPath,
                            loc.cgi_pass.at(ext)
                        );

                        cgi.startCGI(
                            c.fd_in,
                            c.fd_out,
                            c.pid
                        );

                        c.cgi_timing =
                            time(NULL);

                        c.is_cgi = 1;


                        /*
                         * CGI stdin writable.
                         */
                        add_epoll(
                            epfd,
                            c.fd_in,
                            1
                        );


                        /*
                         * CGI stdout readable.
                         */
                        add_epoll(
                            epfd,
                            c.fd_out,
                            0
                        );


                        /*
                         * Save original client fd.
                         */
                        int c_original = fd;

                        int tmp_fd_in =
                            c.fd_in;

                        int tmp_fd_out =
                            c.fd_out;


                        /*
                         * Clone CGI state into pipe entries.
                         */
                        clients[tmp_fd_in] = c;
                        clients[tmp_fd_out] = c;

                        clients[tmp_fd_in].fd =
                            c_original;

                        clients[tmp_fd_out].fd =
                            c_original;


                        /*
                         * Original network socket no longer
                         * needs EPOLLIN while CGI runs.
                         */
                        epoll_ctl(
                            epfd,
                            EPOLL_CTL_DEL,
                            fd,
                            NULL
                        );
                    }


                    /*
                     * =================================================
                     * NORMAL HTTP REQUEST
                     * =================================================
                     */
                    else
                    {
                        c.res_buffer =
                            dispatchRequest(
                                c.req,
                                serv
                            );

                        write(
                            fd,
                            c.res_buffer.c_str(),
                            c.res_buffer.size()
                        );

                        removing_client(
                            epfd,
                            fd,
                            clients
                        );
                    }
                }
            }
        }
    }
}