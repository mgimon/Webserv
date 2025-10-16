#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstring>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <list>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"
#include "InitServer.hpp"

namespace utils {

    void printLocation(const LocationConfig *location);
    bool isCompleteRequest(const std::string& str);
    void readFromSocket(t_socket *client_socket, int epoll_fd, std::map<int, t_socket> &clientSockets);

    int respondGet(ServerConfig &serverOne, int client_fd, std::string path, const HttpRequest &http_request, HttpResponse &http_response);
    int respondPost(int client_fd, const HttpRequest &http_request, HttpResponse &http_response);
    int	respond(int client_fd, const HttpRequest &http_request, ServerConfig &serverOne);

    const LocationConfig* locationMatchforRequest(const std::string &request_path, const std::vector<LocationConfig> &locations);
    std::string getErrorPath(ServerConfig &serverOne, int errcode);
    void hardcodeMultipleLocServer(ServerConfig &server);
    bool isDirectory(const std::string& path);
    std::string generateAutoindex(const std::string& dirPath);
    std::string getRedirectMessage(int code);
}

#endif
