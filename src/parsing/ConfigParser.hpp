#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include "ServerConfig.hpp"

class ConfigParser
{
public:
    explicit ConfigParser(const std::string &filename);

    bool parse();

    const std::vector<ServerConfig> &getServers() const;

private:

    std::string               _filename;
    std::vector<std::string>  _tokens;
    size_t                    _pos;
    std::vector<ServerConfig> _servers;

    bool tokenize();

    bool parseServer(ServerConfig &server);

    bool parseLocation(ServerConfig &server, LocationConfig &location);

    const std::string &current() const;
    std::string        consume();
    bool               isEnd()   const;
    bool               expect(const std::string &expected);

    size_t parseSize(const std::string &str) const;
    void   error(const std::string &msg)    const;
};

#endif
