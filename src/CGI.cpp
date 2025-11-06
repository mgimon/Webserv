#include "../include/CGI.hpp"


void execCGI(int *pipe_write, int *pipe_out, char *nameScript, char *pathScript, char **env)
{
	close(pipe_write[1]);
	close(pipe_out[0]);
	
	dup2(pipe_write[0], STDIN_FILENO); // Creo un fd de la tuberia con el numero 0 (Standar Input)
	dup2(pipe_out[1], STDOUT_FILENO); // Creo un fd de la tuberia con el numero 0 (Standar Output);  
	close(pipe_write[0]);
	close(pipe_out[1]);

	if (access("/usr/bin/python3", X_OK) == -1)
		exit(EXIT_FAILURE);
	if (chdir(pathScript) == -1)
		exit(EXIT_FAILURE);
	if (access(nameScript, R_OK | X_OK) == -1)
		exit(EXIT_FAILURE);
	execve("/usr/bin/python3", &nameScript, env);
	exit(EXIT_FAILURE);
}

bool setNonBlockPipe(int fd)
{
	int flags = fcntl(fd, F_GETFL); 
	if (flags == -1)
	{
		close(fd);
		return (false);  //	No tengo claro que deberia hacer aqui, si simplemente hacer return o otra cosa
	}	
	flags |= O_NONBLOCK; 
	if (fcntl(fd, F_SETFL, flags) == -1)
	{
		close(fd);
		return (false);
	}
	return(true);
}

int startCGI(std::string request, char *nameScript, char *pathScript, char **env, t_socket *client_socket, std::map<int, t_fd_data *> &map_fds, std::map<int, pid_t> &map_pids)
{
	int pipe_write[2]; // Padre escribe, hijo lee
	int pipe_read[2]; // Padre lee, hijo escribe

	if (pipe(pipe_write) == -1)
		return (0); // Devolver un 500 error al cliente
	if (pipe(pipe_read))
	{
		close(pipe_write[0]);
		close(pipe_write[1]);
		return (0); // Devolver un 500 error al cliente
	}

	close(pipe_write[0]);
	close(pipe_read[1]);
	if (!setNonBlockPipe(pipe_write[1]) || !setNonBlockPipe(pipe_read[0]))
		return (0); // Devolver un 500 error al cliente
	pid_t pid = fork();
	if (pid == -1)
	{
		close(pipe_write[1]);
		close(pipe_read[0]);
		return (0); // Devolver un 500 error al cliente
	}
	if (pid == 0) // Child process
		execCGI(pipe_write, pipe_read, nameScript, pathScript, env);
	if (request == "GET")
		loadPipesGet();
	else
	{
		// Meter las pipes en sus structs, en el epoll y en el map de pids
		s_CGI_pipe_write *s_pipe_write = new t_CGI_pipe(pipe_write[1], pipe_read[0],pid, client_socket);
		s_CGI_pipe_read *s_CGI_pipe_read = new t_CGI_pipe(pipe_read[0], pid, client_socket);
		t_fd_data *pipe_write_data = new t_fd_data(s_pipe_in, CGI_PIPE_WRITE);
		t_fd_data *pipe_write_data = new t_fd_data(s_pipe_out, CGI_PIPE_READ);

		map_fds.insert(std::make_pair(pipe_write[1], pipe_in_data));
		map_fds.insert(std::make_pair(pipe_read[0], pipe_out_data));
		map_pids.insert(std::make_pair(pid, pipe_write[1]));
	}

	return(1);
}
