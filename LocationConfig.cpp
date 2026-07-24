#include "webserv.hpp"

LocationConfig::LocationConfig()
    : autoindex(false),
      allow_upload(false),
      allow_delete(false),
      redirect_code(0),
      internal(false)
{
}
