#pragma once

#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>

#include "../include/initServer.hpp"
#include "../include/ServerConfig.hpp"

// Global startCGI to match implementation in src/CGI.cpp
int startCGI(const std::string &cgi, const std::string &nameScript, const std::string &pathScript, char **env, const std::string &request, t_server_context &server_context, t_client_socket *client_socket);