#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../include/initServer.hpp"
#include "../include/ServerConfig.hpp"



namespace CGI
{
	int startCGI(char *cgi, char *nameScript, char *pathScript, char **env, std::string request, t_server_context &server_context, t_client_socket *client_socket);
}