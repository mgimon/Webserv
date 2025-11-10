#include "../include/utilsCC.hpp"


std::string UtilsCC::to_stringCC(int num)
{
	std::stringstream ss;
    ss << num;
	
	return(ss.str());
}

void UtilsCC::closeServer(int epoll_fd, std::map<int, t_fd_data*> &map_fds)
{
	close(epoll_fd);
	std::map<int, t_fd_data*>::iterator it = map_fds.begin();
	while (it != map_fds.end())
	{
		int fd = it->first;
		t_fd_data *fd_data = it->second;

		close(fd);
		if (fd_data->type == LISTEN_SOCKET)
			delete(static_cast<t_listen_socket*>(fd_data->data));
		else if(fd_data->type == CLIENT_SOCKET)
			delete(static_cast<t_client_socket*>(fd_data->data));
		delete(fd_data);
		std::map<int, t_fd_data*>::iterator aux = it;
		++it;
		map_fds.erase(aux);
	}
}
/*void UtilsCC::closeServer(int epoll_fd, std::map<int, t_fd_data*> &map_fds, std::map<int, pid_t> &map_pids)
{
	close(epoll_fd);
	std::map<int, t_fd_data*>::iterator it = map_fds.begin();
	while (it != map_fds.end())
	{
		int fd = it->first;
		t_fd_data *fd_data = it->second;

		close(fd);
		if (fd_data->type == LISTEN_SOCKET || fd_data->type == CLIENT_SOCKET)
		{
			delete(static_cast<t_socket*>(fd_data->data));
			delete(fd_data);
		}
		std::map<int, t_fd_data*>::iterator aux = it;
		++it;
		map_fds.erase(aux);
	}
}*/