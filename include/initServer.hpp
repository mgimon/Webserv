#pragma once

#include "../include/ServerConfig.hpp"
#include <netdb.h>
#include <list>
#include <map>
#include <fcntl.h>
#include <sys/epoll.h>

enum SocketType 
{
    LISTEN_SOCKET,
    CLIENT_SOCKET
};

typedef struct s_socket
{
	int				socket_fd;
	ServerConfig	*server;
	SocketType 		type;
	std::string		readBuffer;
}	t_socket;

void initServer(std::vector<ServerConfig> &serverList);