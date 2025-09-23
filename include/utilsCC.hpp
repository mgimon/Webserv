#pragma once

#include <list>
#include "../include/initServer.hpp"
#include "../include/ServerConfig.hpp"

std::string to_stringCC(int num);
void closeListenSockets(std::list<t_socket> &listenSockets);