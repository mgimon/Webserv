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
		//Posible implemenatcion de flag cuando la conexion se cierra para notificarlo
		//Posible mplementacion d etimer extar para mira el total que lleva el proceso abierto
		bool timeout = (time(NULL) - pids_it->second.last_activity_time >= CGI_TIMEOUT);
		if (timeout)
		{
			std::cerr << RED << "CGI closed by TimeOut" << RESET << std::endl;
			kill(pids_it->first, SIGKILL);
			sendInternalError(pids_it->second.client_socket_fd, map_fds);
			UtilsCC::cleanCGI(epoll_fd, pids_it, map_fds, false);
			std::map<pid_t, t_pid_context>::iterator aux_it = pids_it;
			++pids_it;
			map_pids.erase(aux_it);
		}
		else
		{
			++pids_it;
		}
	}
}

void CGIHandler::writeInPipe(t_CGI_pipe_write *s_pipe_write, uint32_t &events, t_server_context &server_context)
{
	std::map<pid_t, t_pid_context>::iterator pids_it = server_context.map_pids.find(s_pipe_write->pid);

	//Si la pipe se ha cerrado antes de acabar de escribir limpiamos
	if (events & (EPOLLHUP | EPOLLERR))
	{
		std::cerr << RED << "CGI closed by error at writePipe" << RESET << std::endl;
		kill(s_pipe_write->pid, SIGKILL);
		sendInternalError(s_pipe_write->client_socket->socket_fd, server_context.map_fds);
		UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds, false);
		server_context.map_pids.erase(pids_it);
		return;
	}
	pids_it->second.last_activity_time = time(NULL);
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
		pids_it->second.write_finished = true;
		epoll_ctl(server_context.epoll_fd, EPOLL_CTL_DEL, s_pipe_write->fd, NULL);
		close(s_pipe_write->fd);
		server_context.map_fds.erase(s_pipe_write->fd);
		delete(s_pipe_write);
	}
}

void CGIHandler::readFromPipe(t_CGI_pipe_read *s_pipe_read, uint32_t &events, t_server_context &server_context)
{
	std::map<pid_t, t_pid_context>::iterator pids_it = server_context.map_pids.find(s_pipe_read->pid);
	//Mirar que hacer cuabo se cierra la pipe pero no hemos acabdod e excribir, logica del error leyendo
	if (events & EPOLLERR)
	{
		std::cerr << RED << "CGI closed by error at readPipe" << RESET << std::endl;
		kill(s_pipe_read->pid, SIGKILL);
		sendInternalError(s_pipe_read->client_socket->socket_fd, server_context.map_fds);
		UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds, false);
		server_context.map_pids.erase(pids_it);
		return;
	}
	pids_it->second.last_activity_time = time(NULL);
	
	char buf[4096];
    ssize_t bytesRead = read(s_pipe_read->fd, buf, sizeof(buf));

    if (bytesRead < 0)
	{
		std::cerr << RED << "CGI closed by error at readPipe" << RESET << std::endl;
		kill(s_pipe_read->pid, SIGKILL);
		sendInternalError(s_pipe_read->client_socket->socket_fd, server_context.map_fds);
		UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds, false);
		server_context.map_pids.erase(pids_it);
		return;
	}
	
	s_pipe_read->client_socket->sendBuffer.append(buf, bytesRead);
    if (bytesRead == 0)
    {
		//NOTA: Mirar si la funcion es complete request o hay que hacer una en concreto para comrobar si se ha leido todo
		if (pids_it->second.write_finished /*&& isCompleteRequest(s_pipe_read->client_socket->readBuffer*/)
		{
			int keepAlive = utils::respondCGI(server_context, s_pipe_read->client_socket);
			if (keepAlive == -1)
				UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds, false);
			else
			{
				s_pipe_read->client_socket->cgi = false;
				s_pipe_read->client_socket->readBuffer.clear();
				s_pipe_read->client_socket->sendBuffer.clear();
				s_pipe_read->client_socket->last_activity_time = time(NULL);
				UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds, true);
			}
			server_context.map_pids.erase(pids_it);
		}
		else // Se ha acabdo de leer antes de acabar de escribir o la request no esta acabada pero se ha acbado de leer. ERROR LOGICO GRAVE, cerramos
		{
			std::cerr << RED << "CGI closed by error at readPipe" << RESET << std::endl;
			kill(s_pipe_read->pid, SIGKILL);
			sendInternalError(s_pipe_read->client_socket->socket_fd, server_context.map_fds);
			UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds, false);
			server_context.map_pids.erase(pids_it);
		}
	}
}




/* 
#define CGI_TIMEOUT_SECONDS 5 // Constante definida para el límite de tiempo

void CGIHandler::monitor(long current_time, int epoll_fd, std::map<int, t_fd_data> &map_fds, std::map<pid_t, t_pid_context> &map_pids)
{
    std::map<pid_t, t_pid_context>::iterator pids_it = map_pids.begin();
    while (pids_it != map_pids.end())
    {
        int client_fd = pids_it->second.client_socket_fd;
        std::map<int, t_fd_data>::iterator client_it = map_fds.find(client_fd);
        
        bool client_found = (client_it != map_fds.end());
        bool timeout = (current_time - pids_it->second.start_time >= CGI_TIMEOUT_SECONDS);
        bool client_wants_cleanup = false;

        // 1. Obtener el estado de desconexión del cliente si aún existe en map_fds
        if (client_found)
        {
            t_client_socket* client_data = static_cast<t_client_socket*>(client_it->second.data);
            
            // Si el cliente se desconectó, es una condición para limpiar.
            if (client_data->disconnected == true)
                client_wants_cleanup = true;
        }

        // 2. Condición de limpieza unificada (Timeout O Cliente Desconectado)
        if (timeout || client_wants_cleanup || !client_found)
        {
            // Determinar la causa del cierre para el mensaje
            std::string reason;
            if (timeout)
                reason = "TimeOut";
            else if (client_wants_cleanup)
                reason = "Client Closed";
            else
                reason = "Internal Error (FD lost)"; // Esto solo debería ocurrir si hay un fallo lógico

            std::cerr << "CGI closed by CGIMonitor: " << reason << std::endl;
            
            // 3. Matar el proceso y enviar error HTTP al cliente (si es posible)
            kill(pids_it->first, SIGKILL);
            sendInternalError(client_fd, map_fds); // Esta función debe manejar si el FD ya no existe
            
            // 4. Limpieza final de recursos (pipes, flag CGI, y potencialmente el socket)
            UtilsCC::cleanCGI(epoll_fd, pids_it, map_fds, false);
            
            // 5. Eliminar la entrada del PID del mapa
            std::map<pid_t, t_pid_context>::iterator aux_it = pids_it;
            ++pids_it;
            map_pids.erase(aux_it);
        }
        else
        {
            // Si no hay necesidad de limpieza, solo avanzamos
            ++pids_it;
        }
    }
}
*/