#include "webserv.hpp"

LocationConfig::LocationConfig()
    : autoindex(false), redirect_code(0), internal(false),
      allow_upload(false), allow_delete(false)
{
}
