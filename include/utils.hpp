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
#include <list>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"
#include "initServer.hpp"

namespace utils {

    void printLocation(const LocationConfig *location);
    bool isCompleteRequest(const std::string& str);
    void readFromSocket(t_socket *client_socket, int epoll_fd, std::map<int, t_socket> &clientSockets);
    //void readFromSocket(t_socket *client_socket, int epoll_fd, std::list<t_socket> &clientSockets);

    int respondGet(int client_fd, std::string path, const HttpRequest &http_request, HttpResponse &http_response);
    int respondPost(int client_fd, const HttpRequest &http_request, HttpResponse &http_response);
    int	respond(int client_fd, const HttpRequest &http_request, ServerConfig &serverOne);

    const LocationConfig* locationMatchforRequest(const std::string &request_path, const std::vector<LocationConfig> &locations);
    void hardcodeMultipleLocServer(ServerConfig &server);
}

#endif
