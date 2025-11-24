#include "../include/CGIHandler.hpp"
void sendInternalError(int client_socket_fd, std::map<int, t_fd_data> &map_fds)
{
	HttpResponse http_response;

	std::map<int, t_fd_data>::iterator fds_it = map_fds.find(client_socket_fd);
	t_client_socket *s_client_socket = static_cast<t_client_socket*>(fds_it->second.data);
	http_response.setError(utils::getErrorPath(s_client_socket->server, 500), 500, "Internal Server Error");
	http_response.respondInClient(client_socket_fd);
}


void CGIHandler::monitor(int epoll_fd, std::map<int, t_fd_data> &map_fds, std::map<pid_t, t_pid_context> &map_pids)
{
	std::map<pid_t, t_pid_context>::iterator pids_it = map_pids.begin();
	while (pids_it != map_pids.end())
	{
		if (pids_it->second.time >= 50)
		{
			//std::cerr << RED << "CGI closed by TimeOut" << RESET << std::endl;
			kill(pids_it->first, SIGKILL);
			sendInternalError(pids_it->second.client_socket_fd, map_fds);
			UtilsCC::cleanCGI(epoll_fd, pids_it, map_fds);
			std::map<pid_t, t_pid_context>::iterator aux_it = pids_it;
			++pids_it;
			map_pids.erase(aux_it);
		}
		else
		{
			pids_it->second.time++;
			++pids_it;
		}
	}
}

void CGIHandler::writeInPipe(t_CGI_pipe_write *s_pipe_write, uint32_t &event, t_server_context &server_context)
{
	std::map<pid_t, t_pid_context>::iterator pids_it = server_context.map_pids.find(s_pipe_write->pid);

	//Si la pipe se ha cerrado antes de acabar de escribir limpiamos
	if (event & (EPOLLHUP | EPOLLERR))
	{
		//std::cerr << RED << "CGI closed by error at writePipe" << RESET << std::endl;
		kill(s_pipe_write->pid, SIGKILL);
		sendInternalError(s_pipe_write->client_socket->socket_fd, server_context.map_fds);
		UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds);
		server_context.map_pids.erase(pids_it);
		return;
	}
	pids_it->second.time = 0;
	//Calcular cuanto vamos a enviar
	size_t remaining = s_pipe_write->content_length - s_pipe_write->sended;
	size_t send_length = (remaining >= 4096) ? 4096 : remaining;
	//Calcular a partir de donde hemos de escribir
	const void *pos = s_pipe_write->request_body.c_str() + s_pipe_write->sended;

	ssize_t bytesSend = write(s_pipe_write->fd, pos, send_length);
	if (bytesSend <= 0)
		return;
	
	// Si se ha escrito incrementamos el contador de bytes enviados
	s_pipe_write->sended += bytesSend;
	if (s_pipe_write->sended >= s_pipe_write->content_length)
	{
		epoll_ctl(server_context.epoll_fd, EPOLL_CTL_DEL, s_pipe_write->fd, NULL);
		close(s_pipe_write->fd);
		server_context.map_fds.erase(s_pipe_write->fd);
		delete(s_pipe_write);
	}
	
	// NOTA IMPORTANTE: No se que pasa si el content length es 0, no se si es posible y no lo he controlado, 
	// mirar por si acaso. Tambien, en este codigo se da por hecho que content-length es el tamano del body limpio.
	// Confirmar que es asi lo del content-length
}

void CGIHandler::readFromPipe(t_CGI_pipe_read *s_pipe_read, uint32_t &event, t_server_context &server_context)
{
	std::map<pid_t, t_pid_context>::iterator pids_it = server_context.map_pids.find(s_pipe_read->pid);

	//Si la pipe se ha cerrado antes de acabar de leer limpiamos
	if (event & (EPOLLHUP | EPOLLERR))
	{
		//std::cerr << RED << "CGI closed by error at readPipe" << RESET << std::endl;
		kill(s_pipe_read->pid, SIGKILL);
		sendInternalError(s_pipe_read->client_socket->socket_fd, server_context.map_fds);
		UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds);
		server_context.map_pids.erase(pids_it);
		return;
	}
	pids_it->second.time = 0;
	
	char buf[4096];
    ssize_t bytesRead = read(s_pipe_read->fd, buf, sizeof(buf));

    if (bytesRead < 0)
		return;
	
	s_pipe_read->client_socket->sendBuffer.append(buf, bytesRead);
    if (bytesRead == 0)
    {
		//NOTA: Mirar si la funcion es complete request o hay que hacer una en concreto para comrobar si se ha leido todo
		if (pids_it->second.write_finished /*&& isCompleteRequest(s_pipe_read->client_socket->readBuffer*/)
		{
			//Enviar response
		}
		else // Se ha acabdo de leer antes de acabar de escribir o la request no esta acabada pero se ha acbado de leer. ERROR LOGICO GRAVE, cerramos
		{
			//std::cerr << RED << "CGI closed by error at readPipe" << RESET << std::endl;
			kill(s_pipe_read->pid, SIGKILL);
			sendInternalError(s_pipe_read->client_socket->socket_fd, server_context.map_fds);
			UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds);
			server_context.map_pids.erase(pids_it);
		}
	}
}
