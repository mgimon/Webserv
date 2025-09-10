#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"

namespace utils {

    int respondGet(int client_fd, const HttpRequest &http_request, bool &keep_alive);
    int	respond(int client_fd, const HttpRequest &http_request, ServerConfig &serverOne, bool &keep_alive);

    const LocationConfig* locationMatchforRequest(const std::string &request_path, const std::vector<LocationConfig> &locations);
    void hardcodeMultipleLocServer(ServerConfig &server);
}

#endif
