#pragma once

#include "../include/ServerConfig.hpp"
#include <netdb.h>
#include <list>
#include <map>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cerrno>

#define MAX_EVENTS 512

enum FDType 
{
    LISTEN_SOCKET,
    CLIENT_SOCKET,
	CGI_PIPE_IN,
	CGI_PIPE_OUT
};

typedef struct s_socket
{
	int				socket_fd;
	ServerConfig&	server;
	std::string		readBuffer;

	//Constructor
	s_socket(int fd, ServerConfig& srv, const std::string& buffer) : 
        socket_fd(fd), 
        server(srv), // Inicializa la referencia correctamente
        readBuffer(buffer) {}
}	t_socket;

typedef struct s_fd_data
{
	void*	data;
	FDType 	type;

	s_fd_data(void* ptr, FDType tp) : 
        data(ptr), 
        type(tp) {}
}	t_fd_data;

void initServer(std::vector<ServerConfig> &serverList);
