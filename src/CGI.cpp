#include "../include/CGI.hpp"


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

int startCGI(char *nameScript, char *pathScript, char **env)
{
	int pipe_in[2]; // Padre escribe, hijo lee
	int pipe_out[2]; // Hijo escribe, padre lee

	pipe(pipe_in);
	pipe(pipe_out);

	close(pipe_in[0]);
	close(pipe_out[1]);
	if (!setNonBlockPipe(pipe_in[1]) || !setNonBlockPipe(pipe_out[0]))
		return ; // NOTA: Falta decidir que hacer en caso de error

	pid_t pid = fork();
	if (pid == -1)
	{
		close(pipe_in[1]);
		close(pipe_out[0]);
		return(0); // NOTA: Falta decidir que hacer en caso de error
	}
	if (pid == 0) // Child process
	{
		close(pipe_in[1]);
		close(pipe_out[0]);
		
		dup2(pipe_in[0], STDIN_FILENO); // Creo un fd de la tuberia con el numero 0 (Standar Input)
		dup2(pipe_out[1], STDOUT_FILENO); // Creo un fd de la tuberia con el numero 0 (Standar Output);  
		close(pipe_in[0]);
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
	// Meter las pipes en sus struct y en el epoll, y en el map de pids


}