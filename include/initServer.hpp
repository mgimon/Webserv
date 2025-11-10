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
	CGI_PIPE_WRITE,
	CGI_PIPE_READ
};

typedef struct s_CGI_pipe_read
{
	int fd;
	pid_t pid;
	t_client_socket *client_socket;

	s_CGI_pipe_read(int fd_in, pid_t pid_CGI, t_client_socket *conexion_socket) : 
        fd(fd_in),
        pid(pid_CGI),
		client_socket(conexion_socket) {}
}	t_CGI_pipe;

typedef struct s_CGI_pipe_write
{
	int fd;
	int fd_pipe_read;
	pid_t pid;
	t_client_socket *client_socket;

	s_CGI_pipe_write(int fd_in, int fd_out, pid_t pid_CGI, t_client_socket *conexion_socket) : 
        fd(fd_in),
		fd_pipe_read(fd_out), 
        pid(pid_CGI),
		client_socket(conexion_socket) {}
}	t_CGI_pipe;

typedef struct s_client_socket
{
	int				socket_fd;
	ServerConfig&	server;
	std::string		readBuffer;

	//Constructor
	s_client_socket(int fd, ServerConfig& srv, const std::string& buffer) : 
        socket_fd(fd), 
        server(srv), // Inicializa la referencia correctamente
        readBuffer(buffer) {}
}	t_client_socket;

typedef struct s_listen_socket
{
	int				socket_fd;
	ServerConfig&	server;

	//Constructor
	s_listen_socket(int fd, ServerConfig& srv) : 
        socket_fd(fd), 
        server(srv) {} // Inicializa la referencia correctamente
}	t_listen_socket;

typedef struct s_fd_data
{
	void*	data;
	FDType 	type;

	s_fd_data(void* ptr, FDType tp) : 
        data(ptr), 
        type(tp) {}
}	t_fd_data;

typedef struct s_server_context 
{
    int epoll_fd;
    std::map<int, t_fd_data *> &map_fds;
    std::map<int, pid_t> &map_pids;
}	t_server_context;

void initServer(std::vector<ServerConfig> &serverList);