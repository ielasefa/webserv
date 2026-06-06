#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "webserv.hpp"

#include <string>
#include <map>
#include <vector>

struct HttpRequest;

class CGIHandler
{
public:

    CGIHandler(const HttpRequest &request,
               const std::string &script_path,
               const std::string &cgi_executable);

    std::string execute();

private:

    HttpRequest _request;
    std::string _script_path;
    std::string _cgi_executable;

    char      **buildEnv()                                  const;
    void        freeEnv(char **env)                         const;
    std::string unchunkBody(const std::string &chunked)     const;
    std::string wrapResponse(const std::string &cgi_output) const;
};

#endif
