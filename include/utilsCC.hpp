#pragma once

#include "../include/initServer.hpp"
#include "../include/ServerConfig.hpp"

std::string to_stringCC(int num);
void closeListenSockets(std::list<t_socket> &listenSockets);
void closeServer(int epoll_fd, std::map<int, t_socket> &clientSockets, std::list<t_socket> &listenSockets);