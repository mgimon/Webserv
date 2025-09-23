#include "../include/initServer.hpp"
#include "../include/utils.hpp"
#include "../include/utilsCC.hpp"

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
		rcode = getaddrinfo(NULL, to_stringCC(listen.port).c_str(), &hints, &addrinfo_list);
	else
		rcode = getaddrinfo(listen.host.c_str(), to_stringCC(listen.port).c_str(), &hints, &addrinfo_list);

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
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1){ // Permitimos que otro programa pueda usar el puerto mientras el socket se esta cerrando
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
		throw std::runtime_error("Error binding socket"); //NOTA: Printar errno
	
	if (listen(socket_fd, listen_conf.backlog) == -1)
		throw std::runtime_error("Error putting socket to listen"); //NOTA: Printar errno
	return(socket_fd);
}


std::vector<t_socket> loadListenSockets(std::vector<ServerConfig> &serverList)
{
	std::vector<t_socket> listenSockets;
	for(std::vector<ServerConfig>::iterator serv_it = serverList.begin(); serv_it != serverList.end(); ++serv_it)
	{
		std::vector<t_listen> listens = serv_it->getListens();
		for(std::vector<t_listen>::iterator list_it = listens.begin(); list_it != listens.end(); ++list_it)
		{
			try
			{
				int socket_fd = createListenSocket(*list_it);
				t_socket socket = {socket_fd, *serv_it, LISTEN_SOCKET};
				listenSockets.push_back(socket);
			}
			catch(const std::exception& e)
			{
				closeListenSockets(listenSockets);
				throw;
			}
	}
	}
	return(listenSockets);
}

int init_epoll(std::vector<t_socket> &listenSockets)
{
	int fd = epoll_create1(0);
	if (fd == -1)
	{
		closeListenSockets(listenSockets);
		throw std::runtime_error("Epoll error"); //NOTA: Printar errno
	}
	for(std::vector<t_socket>::iterator it = listenSockets.begin(); it != listenSockets.end(); ++it)
	{
		epoll_event ev; 

		//NOTA: MIRAR POSIBLE IMPLEMENTACION DE edge-triggered epoll(EPOLLET)
		ev.events = EPOLLIN; // Para que epoll nos notifique cuando se intente leer del fd (aceptar una conexion cuenta como leer)
		ev.data.ptr = &(*it);
		if (epoll_ctl(fd, EPOLL_CTL_ADD, it->socket_fd, &ev) == -1) // Añadimos los sockets de escucha al epoll 
		{
			closeListenSockets(listenSockets);
			throw std::runtime_error("Epoll ctl error"); //NOTA: Printar errno
		}
	}
	return(fd);
}

void initServer(std::vector<ServerConfig> &serverList)
{
	std::vector<t_socket> listenSockets = loadListenSockets(serverList);
	std::vector<t_socket> clientSockets;
	epoll_event event;
	//ServerConfig serverPrueba = serverList[0];
	//utils::hardcodeMultipleLocServer(serverPrueba);
	
	int epoll_fd = init_epoll(listenSockets);
	
	while(true)
	{
		epoll_wait(epoll_fd, &event, 1, -1);
		t_socket *socket = static_cast<t_socket*>(event.data.ptr);
		if (socket->type == LISTEN_SOCKET)
		{
			sockaddr client_addr;
			socklen_t client_addr_size = sizeof(client_addr);
			int client_fd = accept(socket->socket_fd, &client_addr, &client_addr_size);
			// Gestion de errores del accept
			
			t_socket client_socket = {client_fd, socket->server, CLIENT_SOCKET};
			epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.ptr = &client_socket ;  // Crear t_socket y meterlo en la lista de clientes
			if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
			{
				close(client_fd);
				break;
			}
			// Anadir cliente
			std::vector<t_socket> clientSockets;
			clientSockets.push_back(client_socket);
		}
		else
		{	
			HttpRequest http_request(socket->socket_fd);
			bool keep_alive = true;
			utils::respond(socket->socket_fd, http_request, socket->server, keep_alive);
			
			// Disconect client
			// epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket->socket_fd, &ev)
			// clientSockets.delete()
			// close(socket->socket_fd);
		}
	}
}

/*bool keep_alive = true;
int client_fd = accept(listenSockets[0].socket_fd, &client_addr, &client_addr_size);
if (client_fd == -1)
	throw std::runtime_error("Accept failed");

HttpRequest http_request(client_fd);
http_request.printRequest();
utils::respond(client_fd, http_request, serverPrueba, keep_alive);

close(client_fd);
std::cout << "Closed client fd: " << client_fd << std::endl;*/