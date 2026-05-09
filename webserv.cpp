

#include "webserv.hpp"
const int   BACKLOG             = 128;
const int   POLL_TIMEOUT_MS     = -1;
const int   CGI_TIMEOUT_S       = 10;
const char *DEFAULT_CONFIG_PATH = "webserv.conf";
const int   READ_BUFFER_SIZE    = 4096;
