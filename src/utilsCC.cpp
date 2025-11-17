#include "../include/utilsCC.hpp"


std::string UtilsCC::to_stringCC(int num)
{
	std::stringstream ss;
    ss << num;
	
	return(ss.str());
}

// Version para cerrar si hay error mientras creamos listen scokets 
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
// Version para cerrar el server por signal
/*void cleanCGI(std::map<pid_t, t_pid_context>::iterator &pid_it, 
			  std::map<int, t_fd_data *> &map_fds)
{
	if (!pid_it->second.write_finished)
	{
		int pipe_write_fd = pid_it->second.pipe_write_fd;
		std::map<int, t_fd_data *>::iterator fds_pipe_write_it = map_fds.find(pipe_write_fd);

        close(pipe_write_fd);
        delete(fds_pipe_write_it->second->data);
        delete(fds_pipe_write_it->second);
        map_fds.erase(fds_pipe_write_it);
	}

	//Liberamos read pipe
	int pipe_read_fd = pid_it->second.pipe_read_fd;
	std::map<int, t_fd_data *>::iterator fds_pipe_read_it = map_fds.find(pipe_read_fd);

	close(pipe_read_fd);
	delete(fds_pipe_read_it->second->data);
	delete(fds_pipe_read_it->second);
	map_fds.erase(fds_pipe_read_it);
}*/

//Version para cerrar por signal
void UtilsCC::closeServer(int epoll_fd, std::map<int, t_fd_data*> &map_fds, std::map<pid_t, t_pid_context> &map_pids)
{
	close(epoll_fd);
	std::map<pid_t, t_pid_context>::iterator pids_it = map_pids.begin();
	while (pids_it != map_pids.end())
	{
		kill(pids_it->first, SIGKILL);
		//cleanCGI(pids_it, map_fds);
		std::map<pid_t, t_pid_context>::iterator aux_it = pids_it;
		++pids_it;
		map_pids.erase(aux_it);
	}
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

// Version para limpiar la data cuando hay timeOut
/*void UtilsCC::cleanCGI(int epoll_fd ,std::map<pid_t, t_pid_context>::iterator &pid_it, 
			  std::map<int, t_fd_data *> &map_fds)
{
	// Liberamos write pipe
	if (!pid_it->second.write_finished)
	{
		int pipe_write_fd = pid_it->second.pipe_write_fd;
		std::map<int, t_fd_data *>::iterator fds_pipe_write_it = map_fds.find(pipe_write_fd);

		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pipe_write_fd, NULL);
        close(pipe_write_fd);
        delete(fds_pipe_write_it->second->data);
        delete(fds_pipe_write_it->second);
        map_fds.erase(fds_pipe_write_it);
	}

	// Liberamos read pipe
	int pipe_read_fd = pid_it->second.pipe_read_fd;
	std::map<int, t_fd_data *>::iterator fds_pipe_read_it = map_fds.find(pipe_read_fd);

	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pipe_read_fd, NULL);
	close(pipe_read_fd);
	delete(fds_pipe_read_it->second->data);
	delete(fds_pipe_read_it->second);
	map_fds.erase(fds_pipe_read_it);

}*/

//Liberamos client
/*int client_fd = pid_it->second.client_socket_fd;
std::map<int, t_fd_data *>::iterator fds_client_it = map_fds.find(client_fd);
epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
close(client_fd);
delete(fds_client_it->second->data);
delete(fds_client_it->second);
map_fds.erase(client_fd);*/
