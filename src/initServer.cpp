#include "../include/initServer.hpp"
#include "../include/utils.hpp"
#include "../include/utilsCC.hpp"
#include "../include/Signals.hpp" 
#include "../include/CGIHandler.hpp"

// Devuelve una lista de configuraciones posibles para un listen socket
static addrinfo *getAddrinfoList(t_listen listen)
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

static int createListenSocket(t_listen listen_conf)
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

		if (bind(socket_fd, node->ai_addr, node->ai_addrlen) == 0) // Assignamos la configuracion al socket
			break;
		close(socket_fd);
	}
	freeaddrinfo(addrinfo_list);
	if (node == NULL)
		throw std::runtime_error(strerror(errno));
	
	if (listen(socket_fd, listen_conf.backlog) == -1)
	{
		close(socket_fd);
		throw std::runtime_error(strerror(errno));
	}
	return(socket_fd);
}

static void addListenSocket(int epoll_fd, int socket_fd, ServerConfig &server, std::map<int, t_fd_data> &map_fds)
{
	epoll_event ev;
	t_listen_socket *socket = NULL;
	bool epoll_inserted = false;

	try
	{
		socket = new t_listen_socket(socket_fd, server);
		t_fd_data fd_data = {socket, LISTEN_SOCKET};
		// Añadimos el socket al epoll
		ev.events = EPOLLIN | EPOLLERR; // Para que epoll nos notifique cuando se intente leer del fd (aceptar una conexion cuenta como leer)
		ev.data.fd = socket_fd;
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
		throw ; // Relanza la exception capturada
	}
}

static void loadListenSockets(std::vector<ServerConfig> &serverList, int epoll_fd, std::map<int, t_fd_data> &map_fds)
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

static void addClientSocket(int epoll_fd, int client_fd, ServerConfig &server, std::map<int, t_fd_data> &map_fds)
{
	epoll_event ev;
	t_client_socket *client_socket = NULL;
	bool epoll_inserted = false;

	try
	{
		client_socket = new t_client_socket(client_fd, server);
		t_fd_data fd_data = {client_socket, CLIENT_SOCKET};
		// Añadimos el socket al epoll
		ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
		ev.data.fd = client_fd;
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
	}
}

static void createClientSocket(t_listen_socket *listen_socket, uint32_t &events, t_server_context &server_context)
{
	//Procesamos todas las peticiones al client socket
	sockaddr client_addr;
	socklen_t client_addr_size = sizeof(client_addr);

	if (events & EPOLLERR)
	{
		UtilsCC::closeServer(server_context.epoll_fd, server_context.map_fds, server_context.map_pids);
		throw std::runtime_error(strerror(errno));
	}
	int client_fd = accept(listen_socket->socket_fd, &client_addr, &client_addr_size); // Al aceptar la conexion, se crea socket especifico para este cliente
	if (client_fd == -1)
	{
		UtilsCC::closeServer(server_context.epoll_fd, server_context.map_fds, server_context.map_pids);
		throw std::runtime_error(strerror(errno));
	}

	if (!UtilsCC::addFlagsFd(client_fd))
	{
		close(client_fd);
		return;
	}
	addClientSocket(server_context.epoll_fd, client_fd, listen_socket->server, server_context.map_fds);
}

bool addFlagToEpollFd(int epoll_fd)
{
	int descriptor_flags = fcntl(epoll_fd, F_GETFD);  // Obtenemos las description flags del fd con F_GETFD
	if (descriptor_flags == -1)
		return (false);
	descriptor_flags |= FD_CLOEXEC; // Le añadimos FD_CLOEXEC para que, si se crea una copia en un child process, se cierre al hacer un execv()
	if (fcntl(epoll_fd, F_SETFD, descriptor_flags) == -1) // Usamos F_SETFD para assignarle las nuevas flags
		return (false);
	return(true);
}

void initServer(std::vector<ServerConfig> &serverList)
{
	std::map<int, t_fd_data> map_fds;
	std::map<pid_t, t_pid_context> map_pids;

	int epoll_fd = epoll_create(1);
	if (epoll_fd == -1 || !addFlagToEpollFd(epoll_fd))
		throw std::runtime_error(strerror(errno));
 	
	loadListenSockets(serverList, epoll_fd, map_fds);
	
	epoll_event events[MAX_EVENTS];
	t_server_context server_context = {epoll_fd, map_fds, map_pids};

	//Configuramos las dos señales para cerrar el server
	signal(SIGINT, Signals::signalHandler);
	signal(SIGTERM, Signals::signalHandler);
	while(Signals::running)
	{
		CGIHandler::monitor(epoll_fd, map_fds, map_pids);
		UtilsCC::monitorKA(epoll_fd, map_fds);
		
		int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 500);
		if (n_events == -1)
		{
			// Si se recibe la señal para cerrar el server, sal del bucle de forma controlada y si Signals::running se ha cambiado se cverrara el servidor 
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
			std::map<int, t_fd_data>::iterator it = map_fds.find(events[i].data.fd);
			//Comprobamos si un evento anterior ha liberado data de este evento
			if (it == map_fds.end())
				continue;

			t_fd_data fd_data = it->second;
			if (fd_data.type == LISTEN_SOCKET)
				createClientSocket(static_cast<t_listen_socket *>(fd_data.data), events[i].events, server_context);
			else if (fd_data.type == CLIENT_SOCKET)
				utils::handleClientSocket(fd_data, events[i].events, server_context);
			else if (fd_data.type == CGI_PIPE_WRITE)
				CGIHandler::writeInPipe(static_cast<t_CGI_pipe_write *>(fd_data.data), events[i].events, server_context);
			else if (fd_data.type == CGI_PIPE_READ)
				CGIHandler::readFromPipe(static_cast<t_CGI_pipe_read *>(fd_data.data), events[i].events, server_context);
		}
	}
	UtilsCC::closeServer(epoll_fd, map_fds, map_pids);
	std::cout << GREEN << std::endl << "Server closed" << RESET << std::endl;
}

