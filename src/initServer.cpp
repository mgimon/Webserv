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
		int flags = fcntl(socket_fd, F_GETFL); // Obtenemos las flags del fd con F_GETFL
		if (flags == -1){
			close(socket_fd);
			continue;
		}	
		flags |= O_NONBLOCK | FD_CLOEXEC; // Le añadimos O_NONBLOCK para hacerlo non-blocking y FD_CLOEXEC para que, si se crea una copia en un child process, se cierre al hacer un execv()
		if (fcntl(socket_fd, F_SETFL, flags) == -1){ // Usamos F_SETFL para assignarle las nuevas flags
			close(socket_fd);
			continue;
		}
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

void addListenSocket(int epoll_fd, t_listen_socket *listenSocket, std::map<int, t_fd_data*> &map_fds)
{
	epoll_event ev;

	// Añadimos el socket al map de fds
	t_fd_data *fd_data = new t_fd_data(listenSocket, LISTEN_SOCKET);
	map_fds.insert(std::make_pair(listenSocket->socket_fd, fd_data));
	
	//NOTA: MIRAR POSIBLE IMPLEMENTACION DE edge-triggered epoll(EPOLLET)
	// Añadimos el socket al epoll
	ev.events = EPOLLIN; // Para que epoll nos notifique cuando se intente leer del fd (aceptar una conexion cuenta como leer)
	ev.data.ptr = fd_data;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenSocket->socket_fd, &ev) == -1)
		throw std::runtime_error(strerror(errno));
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
				t_listen_socket *socket = new t_listen_socket(socket_fd, *serv_it);
				addListenSocket(epoll_fd, socket, map_fds);
			}
			catch(const std::exception& e)
			{
				UtilsCC::closeServer(epoll_fd, map_fds);
				throw std::runtime_error(strerror(errno));
			}
		}
	}
}

void addClientSocket(int epoll_fd, t_client_socket *clientSocket, std::map<int, t_fd_data*> &map_fds)
{
	epoll_event ev;

	t_fd_data *fd_data = new t_fd_data(clientSocket, CLIENT_SOCKET);

	// Añadimos el socket al epoll
	ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
	ev.data.ptr = fd_data;
	//Si no se añadido al epoll, lo sacamos del map y liberamos la memoria que acabamos de assignar con new
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientSocket->socket_fd, &ev) == -1)
	{
		close(clientSocket->socket_fd);
		delete(clientSocket);
		delete(fd_data);
		std::cerr << strerror(errno) << std::endl;
	}
	else // Añadimos el socket al map de fds si no ha fallado al meterlo en el epoll
		map_fds.insert(std::make_pair(clientSocket->socket_fd, fd_data));
}

void createClientSocket(t_listen_socket *listen_socket, int epoll_fd, std::map<int, t_fd_data *> &map_fds)
{
	sockaddr client_addr;
	socklen_t client_addr_size = sizeof(client_addr);

	listen_socket->server.print();
	int client_fd = accept(listen_socket->socket_fd, &client_addr, &client_addr_size); // Al aceptar la conexion, se crea socket especifico para este cliente
	//if (client_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) Error muy especifico para un level triggered epoll (nivel que falle un write), no se si ponerlo, ademas cero que no esta permitido por el saubject
	//	return;
	if (client_fd == -1)
	{
		UtilsCC::closeServer(epoll_fd, map_fds);
		throw std::runtime_error(strerror(errno));
	}

	// Hacemos el socket non-blocking
	int flags = fcntl(client_fd, F_GETFL); 
	if (flags == -1)
	{
		close(client_fd);
		return;
	}	
	flags |= O_NONBLOCK | FD_CLOEXEC; 
	if (fcntl(client_fd, F_SETFL, flags) == -1)
	{
		close(client_fd);
		return;
	}

	t_client_socket *client_socket = new t_client_socket(client_fd, listen_socket->server, "");
	addClientSocket(epoll_fd, client_socket, map_fds);
}

void initServer(std::vector<ServerConfig> &serverList)
{
	std::map<int, t_fd_data *> map_fds;
	std::map<pid_t, t_pid_context> map_pids;
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
		/*std::map<pid_t, t_pid_context>::iterator pids_it = map_pids.begin();
		while (pids_it != map_pids.end())
		{
			if (pids_it->second.time >= 50)
			{
				kill(pids_it->first, SIGKILL);
				//Send error to client
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
		}*/
		
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
			else/*if (fd_data->type == CLIENT_SOCKET)*/
				utils::handleClientSocket(fd_data, server_context, events, i);
			//else if (fd_data->type == CGI_PIPE_IN)
			//else if (fd_data->type == CGI_PIPE_OUT)
		}
	}
	UtilsCC::closeServer(epoll_fd, map_fds, map_pids);
	std::cout << RED << std::endl << "Server closed" << RESET << std::endl;
}
