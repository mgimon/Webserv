#pragma once

#include "initServer.hpp"
#include "ServerConfig.hpp"
#include "CGI.hpp"

namespace UtilsCC
{
	std::string to_stringCC(int num);
	bool addFlagsFd(int fd);
	void closeServer(int epoll_fd, std::map<int, t_fd_data*> &map_fds);
	void closeServer(int epoll_fd, std::map<int, t_fd_data*> &map_fds, std::map<pid_t, t_pid_context> &map_pids);
	void cleanCGI(int epoll_fd, std::map<pid_t, t_pid_context>::iterator &pid_it, std::map<int, t_fd_data *> &map_fds);
}