#pragma once

#include "../include/initServer.hpp"
#include "../include/ServerConfig.hpp"

namespace UtilsCC
{
	std::string to_stringCC(int num);
	void closeServer(int epoll_fd, std::map<int, t_fd_data*> &map_fds);
	void closeServer(int epoll_fd, std::map<int, t_fd_data*> &map_fds, std::map<int, pid_t> &map_pids);
	void cleanCGI(int epoll_fd ,std::map<pid_t, t_pid_context>::iterator &pid_it, std::map<int, t_fd_data *> &map_fds);
}