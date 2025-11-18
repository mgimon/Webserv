#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ServerConfig.hpp"
#include "initServer.hpp"
#include "utilsCC.hpp"


namespace CGI
{
	int startCGI(char *cgi, char *nameScript, char *pathScript, char **env, std::string request, t_server_context &server_context, t_client_socket *client_socket);
}