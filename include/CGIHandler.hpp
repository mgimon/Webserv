#pragma once

#include "CGI.hpp"
#include "utils.hpp"

namespace CGIHandler
{
	void monitor(int epoll_fd, std::map<int, t_fd_data> &map_fds, std::map<pid_t, t_pid_context> &map_pids);
	void writeInPipe(t_CGI_pipe_write *s_pipe_write, uint32_t &event, t_server_context &server_context);
	void readFromPipe(t_CGI_pipe_read *s_pipe_read, uint32_t &event, t_server_context &server_context);
}
