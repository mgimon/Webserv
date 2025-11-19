#include "../include/initServer.hpp"
#include "../include/utils.hpp"
#include "../include/utilsCC.hpp"
#include "../include/Signals.hpp" 

// Devuelve una lista de configuraciones posibles para un listen socket
addrinfo *getAddrinfoList(t_listen listen)
{
	addrinfo *addrinfo_list;
	addrinfo hints;
	int rcode;
	
	memset(&hints, 0, sizeof(hints)); // Llenamos hints de zeros para asegurar que no hay memoria basura
	hints.ai_family = AF_INET;    // Usa IPv4 para la conexion 
	hints.ai_socktype = SOCK_STREAM; // Stream socket (usa TCP como protocolo) 
	hints.ai_flags = AI_PASSIVE; // Para usar todas las interfaces del pc (solo se aplica si pasamos NULL como primer parametro a getaddrinfo)

	if (listen.host == "")
		rcode = getaddrinfo(NULL, UtilsCC::to_stringCC(listen.port).c_str(), &hints, &addrinfo_list);
	else
		rcode = getaddrinfo(listen.host.c_str(), UtilsCC::to_stringCC(listen.port).c_str(), &hints, &addrinfo_list);

	if (rcode != 0)
		throw std::runtime_error(gai_strerror(rcode));
	return (addrinfo_list);
}

int createListenSocket(t_listen listen_conf)
{
	int socket_fd, option;
	addrinfo *addrinfo_list = getAddrinfoList(listen_conf);
	addrinfo *node;

	for (node = addrinfo_list; node != NULL; node = node->ai_next)
	{
		socket_fd = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
		if (socket_fd == -1)
			continue;
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1){ // Permitimos que otro programa pueda usar el puerto mientras el socket se esta cerrando con SO_REUSEADDR
			close(socket_fd);
			continue;
		}
		if (!UtilsCC::addFlagsFd(socket_fd))
		{
			close(socket_fd);
			continue;
		}
		//CODIGO SUBSTITUIDO POR addFlagsFd()
		/*int status_flags = fcntl(socket_fd, F_GETFL); // Obtenemos las status flags del fd con F_GETFL
		if (status_flags == -1){
			close(socket_fd);
			continue;
		}	
		status_flags |= O_NONBLOCK; // Le añadimos O_NONBLOCK para hacerlo non-blocking 
		if (fcntl(socket_fd, F_SETFL, status_flags) == -1){ // Usamos F_SETFL para assignarle las nuevas flags
			close(socket_fd);
			continue;
		}
		int descriptor_flags = fcntl(socket_fd, F_GETFD);  // Obtenemos las description flags del fd con F_GETFD
		if (descriptor_flags == -1){
			close(socket_fd);
			continue;
		}	
		descriptor_flags |= FD_CLOEXEC; // Le añadimos FD_CLOEXEC para que, si se crea una copia en un child process, se cierre al hacer un execv()
		if (fcntl(socket_fd, F_SETFD, descriptor_flags) == -1){ // Usamos F_SETFD para assignarle las nuevas flags
			close(socket_fd);
			continue;
		}*/

		if (bind(socket_fd, node->ai_addr, node->ai_addrlen) == 0) // Assignamos la configuracion al socket
			break;
		close(socket_fd);
	}
	freeaddrinfo(addrinfo_list);
	if (node == NULL)
		throw std::runtime_error(strerror(errno));
	
	if (listen(socket_fd, listen_conf.backlog) == -1)
		throw std::runtime_error(strerror(errno));
	return(socket_fd);
}

void addListenSocket(int epoll_fd, int socket_fd, ServerConfig &server, std::map<int, t_fd_data*> &map_fds)
{
	epoll_event ev;
	t_listen_socket *socket = NULL;
	t_fd_data *fd_data = NULL;
	bool epoll_inserted = false;

	try
	{
		socket = new t_listen_socket(socket_fd, server);
		fd_data = new t_fd_data(socket, LISTEN_SOCKET);
		// NOTA: MIRAR POSIBLE IMPLEMENTACION DE edge-triggered epoll(EPOLLET)
		// Añadimos el socket al epoll
		ev.events = EPOLLIN; // Para que epoll nos notifique cuando se intente leer del fd (aceptar una conexion cuenta como leer)
		ev.data.ptr = fd_data;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1)
			throw std::runtime_error(strerror(errno));
		epoll_inserted = true;
		map_fds.insert(std::make_pair(socket_fd, fd_data));
	}
	catch(const std::exception& e)
	{
		if (epoll_inserted)
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
		close(socket_fd);
		if (socket != NULL)
			delete(socket);
		if (fd_data != NULL)
			delete(fd_data);
		throw ; // Relanza la exception capturada
	}
}

void loadListenSockets(std::vector<ServerConfig> &serverList, int epoll_fd, std::map<int, t_fd_data*> &map_fds)
{
	for (std::vector<ServerConfig>::iterator serv_it = serverList.begin(); serv_it != serverList.end(); ++serv_it)
	{
		std::vector<t_listen> listens = serv_it->getListens();
		for (std::vector<t_listen>::iterator list_it = listens.begin(); list_it != listens.end(); ++list_it)
		{
			try
			{
				int socket_fd = createListenSocket(*list_it);
				addListenSocket(epoll_fd, socket_fd, *serv_it, map_fds);
			}
			catch(const std::exception& e)
			{
				UtilsCC::closeServer(epoll_fd, map_fds);
				throw std::runtime_error(e.what());
			}
		}
	}
}

void addClientSocket(int epoll_fd, int client_fd, ServerConfig &server, std::map<int, t_fd_data*> &map_fds)
{
	epoll_event ev;
	t_client_socket *client_socket = NULL;
	t_fd_data *fd_data = NULL;
	bool epoll_inserted = false;

	try
	{
		client_socket = new t_client_socket(client_fd, server);
		fd_data = new t_fd_data(client_socket, CLIENT_SOCKET);
		// Añadimos el socket al epoll
		ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
		ev.data.ptr = fd_data;
		// Si falla al meterse en el epoll lanzamos una exception y en el catch liberamos la memoria
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
			throw(std::runtime_error(strerror(errno)));
		epoll_inserted = true;
	 	// Añadimos el socket al map de fds si no ha fallado al meterlo en el epoll
		map_fds.insert(std::make_pair(client_fd, fd_data));
	}
	catch(const std::exception& e)
	{
		std::cerr << RED << e.what() << RESET << std::endl;
		if (epoll_inserted)
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
		close(client_fd);
		if (client_socket != NULL)
			delete(client_socket);
		if (fd_data != NULL)
			delete(fd_data);
	}
}

void createClientSocket(t_listen_socket *listen_socket, int epoll_fd, std::map<int, t_fd_data *> &map_fds)
{
	sockaddr client_addr;
	socklen_t client_addr_size = sizeof(client_addr);

	listen_socket->server.print();
	int client_fd = accept(listen_socket->socket_fd, &client_addr, &client_addr_size); // Al aceptar la conexion, se crea socket especifico para este cliente
	//if (client_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) Error muy especifico para un level triggered epoll (nivel que falle un write), no se si ponerlo,
	//	return;
	if (client_fd == -1)
	{
		UtilsCC::closeServer(epoll_fd, map_fds);
		throw std::runtime_error(strerror(errno));
	}

	if (!UtilsCC::addFlagsFd(client_fd))
	{
		close(client_fd);
		return;
	}
	addClientSocket(epoll_fd, client_fd, listen_socket->server, map_fds);
}

void initServer(std::vector<ServerConfig> &serverList)
{
	std::map<int, t_fd_data *> map_fds;
	std::map<pid_t, t_pid_context> map_pids;
	//NOTA: HACER EPOLL_FD FD_CLOEXEC
	int epoll_fd = epoll_create(1);
	if (epoll_fd == -1)
		throw std::runtime_error(strerror(errno));
 	loadListenSockets(serverList, epoll_fd, map_fds);
	
	epoll_event events[MAX_EVENTS];
	t_server_context server_context = {epoll_fd, map_fds, map_pids};
	signal(SIGINT, Signals::signalHandler);
	while(Signals::running)
	{
		//CHECK MAP_PIDS
		std::map<pid_t, t_pid_context>::iterator pids_it = map_pids.begin();
		while (pids_it != map_pids.end())
		{
			if (pids_it->second.time >= 50)
			{
				kill(pids_it->first, SIGKILL);
				//NOTA: Send error to client
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
		
		int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 100);
		if (n_events == -1)
		{
			if (errno == EINTR)
				continue;
			else
			{
				UtilsCC::closeServer(epoll_fd, map_fds, map_pids);
				throw std::runtime_error(strerror(errno));
			}
		}
		for (int i = 0; i < n_events; i++)
		{
			t_fd_data *fd_data = static_cast<t_fd_data *>(events[i].data.ptr);
			if (fd_data->type == LISTEN_SOCKET)
				createClientSocket(static_cast<t_listen_socket *>(fd_data->data), epoll_fd, map_fds);
			else if (fd_data->type == CLIENT_SOCKET)
				utils::handleClientSocket(fd_data, server_context, events, i);
			else if (fd_data->type == CGI_PIPE_WRITE)
			{
				t_CGI_pipe_write *s_pipe_write = static_cast<t_CGI_pipe_write *>(fd_data->data);
				std::map<pid_t, t_pid_context>::iterator pids_it = map_pids.find(s_pipe_write->pid);

				//Calcular cuanto vamos a enviar
				size_t remaining = s_pipe_write->content_length - s_pipe_write->sended;
				size_t send_len = (remaining > 4096) ? 4096 : remaining;

				ssize_t bytesSend = write(s_pipe_write->fd, s_pipe_write->request_body.c_str(), send_len);
				if (bytesSend <= 0)
				{
					kill(s_pipe_write->pid, SIGKILL);
					//NOTA: ENVIAR ERRROR A CLIENTE
					UtilsCC::cleanCGI(epoll_fd, pids_it, map_fds);
					//NECESITO SABER SI EL KEEP ALIVE ESTA PUESTO O NO
					map_pids.erase(pids_it);
					continue;
				}
				// Si se ha ledio reseteamos el time e incrementamos el contador de bytes enviados
				pids_it->second.time = 0;
				s_pipe_write->sended += 4096;
				//Hacer un erase de lo que hemos enviado
				s_pipe_write->request_body.erase(s_pipe_write->request_body.begin() - send_len);
				if (s_pipe_write->sended >= s_pipe_write->content_length)
				{
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, s_pipe_write->fd, NULL);
					close(s_pipe_write->fd);
					map_fds.erase(s_pipe_write->fd);
					delete(s_pipe_write);
					delete(fd_data);
				}
			}
			else if (fd_data->type == CGI_PIPE_READ)
			{

			}
		}
	}
	UtilsCC::closeServer(epoll_fd, map_fds, map_pids);
	std::cout << RED << std::endl << "Server closed" << RESET << std::endl;
}
