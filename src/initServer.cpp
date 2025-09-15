#include "../include/initServer.hpp"
#include "../include/utilsCC.hpp"

addrinfo *getAddrinfoList(t_listen listen)
{
	addrinfo *addrinfo_list;
	addrinfo hints;
	int rcode;
	
	memset(&hints, 0, sizeof(hints)); // Llenamos hints de zeros para asegurar que no hay memoria basura
	hints.ai_family = AF_INET;    // Usa IPv4 para la conexion 
	hints.ai_socktype = SOCK_STREAM; // Stream socket (usa TCP como protocolo) 
	hints.ai_flags = AI_PASSIVE; // Para usar todas las interfaces del pc (solo se aplica si pasamos NULL como primer parametron a getaddrinfo)

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
	addrinfo *addrinfo_list = getAddrinfoList(listen_conf); // Obtenemos una lista de configuraciones posibles para el socket
	addrinfo *node;

	for (node = addrinfo_list; node != NULL; node = node->ai_next)
	{
		socket_fd = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
		if (socket_fd == -1)
			continue;
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) // Permitimos que otro progarma pueda usar el puerto mientars el socket se esta cerrando
			continue;
		if (fcntl() == -1)
		if (bind(socket_fd, node->ai_addr, node->ai_addrlen) == 0) // Assignamos la configuracion al socket
			break;
		close(socket_fd);
	}
	freeaddrinfo(addrinfo_list);
	if (node == NULL)
		throw std::runtime_error("Error binding socket");
	
	if (listen(socket_fd, listen_conf.backlog) == -1)
		throw std::runtime_error("Error putting socket to listen");
	return(socket_fd);
}


std::vector<int> loadListenSockets(const std::vector<ServerConfig> &serverList)
{
	std::vector<int> listenSockets;
	for(std::vector<ServerConfig>::const_iterator serv_it = serverList.begin(); serv_it != serverList.end(); ++serv_it)
	{
		std::vector<t_listen> listens = serv_it->getListens();
		for(std::vector<t_listen>::iterator list_it = listens.begin(); list_it != listens.end(); ++list_it)
		{
			try
			{
				int socket_fd = createListenSocket(*list_it);
				listenSockets.push_back(socket_fd);
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


void initServer(std::vector<ServerConfig> &serverList)
{
	std::vector<int> listenSockets = loadListenSockets(serverList);
	sockaddr client_addr;
	socklen_t client_addr_size = sizeof(client_addr);
	
	while(true)
	{
		int client_fd = accept(listenSockets[0], &client_addr, &client_addr_size);
		if (client_fd == -1)
			throw std::runtime_error("Accept failed");
		
		char buffer[1024];
        int bytes_read = recv(client_fd, buffer, 1024, 0);
        if (bytes_read > 0) {
            std::cout << "Received: " << std::string(buffer, bytes_read) << std::endl;
        }

        close(client_fd);
        std::cout << "Closed client fd: " << client_fd << std::endl;
    }
}