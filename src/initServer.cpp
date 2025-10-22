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

	if (listen.host == "0.0.0.0")
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


std::list<t_socket> loadListenSockets(std::vector<ServerConfig> &serverList)
{
	std::list<t_socket> listenSockets;
	for (std::vector<ServerConfig>::iterator serv_it = serverList.begin(); serv_it != serverList.end(); ++serv_it)
	{
		std::vector<t_listen> listens = serv_it->getListens();
		for (std::vector<t_listen>::iterator list_it = listens.begin(); list_it != listens.end(); ++list_it)
		{
			try
			{
				int socket_fd = createListenSocket(*list_it);
				t_socket socket = {socket_fd, *serv_it, LISTEN_SOCKET, ""};
				listenSockets.push_back(socket);
			}
			catch(const std::exception& e)
			{
				UtilsCC::closeListenSockets(listenSockets);
				throw;
			}
		}
	}
	return (listenSockets);
}

int init_epoll(std::list<t_socket> &listenSockets)
{
	int fd = epoll_create(1);
	if (fd == -1)
	{
		UtilsCC::closeListenSockets(listenSockets);
		throw std::runtime_error(strerror(errno));
	}
	for (std::list<t_socket>::iterator it = listenSockets.begin(); it != listenSockets.end(); ++it)
	{
		epoll_event ev;

		//NOTA: MIRAR POSIBLE IMPLEMENTACION DE edge-triggered epoll(EPOLLET)
		ev.events = EPOLLIN; // Para que epoll nos notifique cuando se intente leer del fd (aceptar una conexion cuenta como leer)
		ev.data.ptr = &(*it);
		if (epoll_ctl(fd, EPOLL_CTL_ADD, it->socket_fd, &ev) == -1)
		{
			UtilsCC::closeListenSockets(listenSockets);
			throw std::runtime_error(strerror(errno));
		}
	}
	return(fd);
}

void createClientSocket(t_socket *listen_socket, int epoll_fd, std::map<int, t_socket> &clientSockets, std::list<t_socket> &listenSockets)
{
	sockaddr client_addr;
	socklen_t client_addr_size = sizeof(client_addr);
	epoll_event ev;

	int client_fd = accept(listen_socket->socket_fd, &client_addr, &client_addr_size); // Al aceptar la conexion, se crea socket especifico para este cliente
	//if (client_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) Error muy especifico para un level triggered epoll (nivel que falle un write), no se si ponerlo
	//	return;
	if (client_fd == -1)
	{
		UtilsCC::closeServer(epoll_fd, clientSockets, listenSockets);
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

	// Añadimos el socket al map de clientScokets
	t_socket client_socket = {client_fd, listen_socket->server, CLIENT_SOCKET, ""};
	std::map<int, t_socket>::iterator it_client_sock = clientSockets.insert(std::make_pair(client_fd, client_socket)).first; // Crear t_socket y meterlo en un map de clientes
	
	// Añadimos el socket al epoll
	ev.events = EPOLLIN;
	ev.data.ptr = &(it_client_sock->second);
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
	{
		UtilsCC::closeServer(epoll_fd, clientSockets, listenSockets);
		throw std::runtime_error(strerror(errno));
	}
}

void initServer(std::vector<ServerConfig> &serverList)
{
	std::list<t_socket> listenSockets = loadListenSockets(serverList);
	std::map<int, t_socket> clientSockets;
	epoll_event events[MAX_EVENTS];

	int epoll_fd = init_epoll(listenSockets);

	signal(SIGINT, Signals::signalHandler);
	while(Signals::running)
	{
		int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (n_events == -1)
		{
			if (errno == EINTR)
				continue;
			else
			{
				UtilsCC::closeServer(epoll_fd, clientSockets, listenSockets);
				throw std::runtime_error(strerror(errno));
			}
		}
		for (int i = 0; i < n_events; i++)
		{
			t_socket *socket = static_cast<t_socket *>(events[i].data.ptr);
			if (socket->type == LISTEN_SOCKET)
				createClientSocket(socket, epoll_fd, clientSockets, listenSockets);
			else
				utils::handleClientSocket(socket, epoll_fd, clientSockets, events, i);
		}
	}
	UtilsCC::closeServer(epoll_fd, clientSockets, listenSockets);
	std::cout << std::endl << "Server closed" << std::endl;
}
