#include "../../webserv.hpp"

LocationConfig::LocationConfig()
    : autoindex(false), has_explicit_root(false), client_max_body_size(0),
      has_client_max_body_size(false), redirect_code(0), internal(false),
      allow_upload(false), allow_delete(false)
{
}
