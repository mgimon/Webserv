#include "../include/CGI.hpp"

int addPipeWrite(int *pipe_write, int *pipe_read, pid_t pid, t_client_socket *client_socket, t_server_context &server_context)
{
	epoll_event ev_pipe_write;
	s_CGI_pipe_write *s_pipe_write = new s_CGI_pipe_write(pipe_write[1], pipe_read[0], pid, client_socket);
	t_fd_data *pipe_write_data = new t_fd_data(s_pipe_write, CGI_PIPE_WRITE);

	ev_pipe_write.events = EPOLLOUT | EPOLLHUP | EPOLLERR;
	ev_pipe_write.data.ptr = pipe_write_data;
	if (epoll_ctl(server_context.epoll_fd, EPOLL_CTL_ADD, pipe_write[1], &ev_pipe_write) == -1)
	{
		//Liberamos el read pipe
		t_fd_data *pipe_read_data = server_context.map_fds.at(pipe_read[0]);
		epoll_ctl(server_context.epoll_fd, EPOLL_CTL_DEL, pipe_read[0], NULL);
		close(pipe_read[0]);
		delete(pipe_read_data->data);
		delete(pipe_read_data);
		server_context.map_fds.erase(pipe_read[0]);
		//Liberamos el write pipe
		close(pipe_write[1]);
		delete(s_pipe_write);
		delete(pipe_write_data);
		std::cerr << strerror(errno) << std::endl;
		return (0); // Devolver un 500 error al cliente
	}
	server_context.map_fds.insert(std::make_pair(pipe_write[1], pipe_write_data));
	server_context.map_pids.insert(std::make_pair(pid, pipe_write[1]));
	return (1);
}

int addPipeRead(int *pipe_write, int *pipe_read, pid_t pid, t_client_socket *client_socket, t_server_context &server_context)
{
	epoll_event ev_pipe_read;

	s_CGI_pipe_read *s_pipe_read = new s_CGI_pipe_read(pipe_read[0], pid, client_socket);
	t_fd_data *pipe_read_data = new t_fd_data(s_pipe_read, CGI_PIPE_READ);
	ev_pipe_read.events = EPOLLIN | EPOLLHUP | EPOLLERR;
	ev_pipe_read.data.ptr = pipe_read_data;
	if (epoll_ctl(server_context.epoll_fd, EPOLL_CTL_ADD, pipe_read[0], &ev_pipe_read) == -1)
	{
		//Liberamos el read pipe y cerrramos el fd de write pipes
		close(pipe_write[1]);
		close(pipe_read[0]);
		delete(s_pipe_read);
		delete(pipe_read_data);
		std::cerr << strerror(errno) << std::endl;
		return (0); // Devolver un 500 error al cliente
	}
	server_context.map_fds.insert(std::make_pair(pipe_read[0], pipe_read_data));
	return(1);
}

void execCGI(int *pipe_write, int *pipe_read, char *cgi, char *nameScript, char *pathScript, char **env) // cgi = ruat del ejecutable cgi
{	
	char *argv[3];

	dup2(pipe_write[0], STDIN_FILENO); // Creo un fd de la tuberia con el numero 0 (Standar Input)
	dup2(pipe_read[1], STDOUT_FILENO); // Creo un fd de la tuberia con el numero 1 (Standar Output);  
	//Cerramos los fds que acabamos de duplicar
	close(pipe_write[0]);
	close(pipe_read[1]);
	//Cerramos los fds de los otros exetremo de las tuberias que no necesitamos en el hijo
	close(pipe_write[1]);
	close(pipe_read[0]);

	if (access(cgi, X_OK) == -1)
		exit(EXIT_FAILURE);
	if (chdir(pathScript) == -1)
		exit(EXIT_FAILURE);
	if (access(nameScript, R_OK | X_OK) == -1)
		exit(EXIT_FAILURE);
	//Creamos el array del comando
	argv[0] = cgi;
	argv[1] = nameScript;
	argv[2] = NULL;
	//Ejecutamos
	execve(cgi, argv, env);
	exit(EXIT_FAILURE);
}

void closePipe(int *pipe)
{
	close(pipe[0]);
	close(pipe[1]);
}

bool setNonBlockPipe(int fd)
{
	int flags = fcntl(fd, F_GETFL); 
	if (flags == -1)
	{
		close(fd);
		return (false);  //	Devolver un 500 error al cliente
	}	
	flags |= O_NONBLOCK; 
	if (fcntl(fd, F_SETFL, flags) == -1)
	{
		close(fd);
		return (false); // Devolver un 500 error al cliente
	}
	return(true);
}

int startCGI(char *cgi, char *nameScript, char *pathScript, char **env, std::string request, t_server_context &server_context, t_client_socket *client_socket)
{
	int pipe_write[2]; // Padre escribe, hijo lee
	int pipe_read[2]; // Padre lee, hijo escribe
	epoll_event ev_pipe_read;

	if (pipe(pipe_write) == -1)
		return (0); // Devolver un 500 error al cliente
	if (pipe(pipe_read))
	{
		closePipe(pipe_write);
		return (0); // Devolver un 500 error al cliente
	}
	if (!setNonBlockPipe(pipe_write[1]) || !setNonBlockPipe(pipe_read[0]))
		return (0); // Devolver un 500 error al cliente
	
	pid_t pid = fork();
	if (pid == -1)
	{
		closePipe(pipe_write);
		closePipe(pipe_read);
		return (0); // Devolver un 500 error al cliente
	}
	if (pid == 0) // Child process
		execCGI(pipe_write, pipe_read, cgi, nameScript, pathScript, env);
	//Cerramos los fds de los otros exetremo de las tuberias que no necesitamos en el padre
	close(pipe_write[0]);
	close(pipe_read[1]);
	
	if (addPipeRead(pipe_write, pipe_read, pid, client_socket, server_context) == 0)
		return (0);

	//Si el metodo es POST, usamos las dos pipes, si no cerramos la de lectura
	if (request == "POST")
	{
		if (addPipeWrite(pipe_write, pipe_read, pid, client_socket, server_context) == 0)
			return (0);
	}
	else
	{
		close(pipe_write[1]);
		server_context.map_pids.insert(std::make_pair(pid, pipe_read[0]));
	}
	return(1);
}
