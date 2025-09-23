#pragma once

#include "../include/ServerConfig.hpp"
#include "../include/initServer.hpp"

std::string to_stringCC(int num);
void closeListenSockets(std::vector<t_socket> &listenSockets);