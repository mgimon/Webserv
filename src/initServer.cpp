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
                t_socket socket = {socket_fd, &(*serv_it), LISTEN_SOCKET, ""};
                listenSockets.push_back(socket);
            }
            catch(const std::exception& e)
            {
                closeListenSockets(listenSockets);
                throw;
            }
        }
    }
    return (listenSockets);
}

int init_epoll(std::list<t_socket> &listenSockets)
{
	int fd = epoll_create1(0);
	if (fd == -1)
	{
		closeListenSockets(listenSockets);
		throw std::runtime_error("Epoll error"); //NOTA: Printar errno
	}
    for (std::list<t_socket>::iterator it = listenSockets.begin(); it != listenSockets.end(); ++it)
    {
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.ptr = &(*it);
        if (epoll_ctl(fd, EPOLL_CTL_ADD, it->socket_fd, &ev) == -1)
        {
            closeListenSockets(listenSockets);
            throw std::runtime_error("Epoll ctl error");
        }
    }
	return(fd);
}

/*void initServer(std::vector<ServerConfig> &serverList)
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
			if (client_fd == -1)
			{
				perror("accept");
				continue; // no abortes el loop entero
			}

			// Guarda el cliente en el vector global (de arriba, fuera del while)
			clientSockets.push_back((t_socket){client_fd, socket->server, CLIENT_SOCKET});
			t_socket* client_ptr = &clientSockets.back();

			// Registra el cliente en epoll
			epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.ptr = client_ptr;
			if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
			{
				perror("epoll_ctl add client");
				close(client_fd);
				clientSockets.pop_back();
			}
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
}*/



void initServer(std::vector<ServerConfig> &serverList)
{
    std::list<t_socket> listenSockets = loadListenSockets(serverList);
    std::list<t_socket> clientSockets; // lista
    epoll_event event;

    int epoll_fd = init_epoll(listenSockets);

    while (true)
    {
        int n = epoll_wait(epoll_fd, &event, 1, -1);
        if (n == -1)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }
        if (n == 0)
            continue;

        t_socket *socket = static_cast<t_socket *>(event.data.ptr);
        if (!socket)
            continue;

        if (socket->type == LISTEN_SOCKET)
        {
            sockaddr client_addr;
            socklen_t client_addr_size = sizeof(client_addr);
            int client_fd = accept(socket->socket_fd, &client_addr, &client_addr_size);
            if (client_fd == -1)
            {
                perror("accept");
                continue;
            }

            t_socket new_client = {client_fd, socket->server, CLIENT_SOCKET, ""};
            clientSockets.push_back(new_client);
            t_socket *client_ptr = &clientSockets.back(); // puntero estable
            epoll_event ev;
            ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
            ev.data.ptr = client_ptr;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
            {
                perror("epoll_ctl add client");
                close(client_fd);
                clientSockets.pop_back();
                continue;
            }
        }
        else
        {
            t_socket *client_socket = socket;

            if (event.events & (EPOLLHUP | EPOLLERR))
            {
                int fd = client_socket->socket_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                for (std::list<t_socket>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it)
                {
                    if (&(*it) == client_socket)
                    {
                        clientSockets.erase(it);
                        break;
                    }
                }
                continue;
            }

			utils::readFromSocket(client_socket, epoll_fd, clientSockets);

			if (utils::isCompleteRequest(client_socket->readBuffer))
			{
				// manejar petición HTTP
				HttpRequest http_request(client_socket->readBuffer);
				http_request.printRequest();

				if (utils::respond(client_socket->socket_fd, http_request, *(client_socket->server)) == -1)
				{
					int fd = client_socket->socket_fd;
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
					close(fd);
					for (std::list<t_socket>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it)
					{
						if (&(*it) == client_socket)
						{
							clientSockets.erase(it);
							break;
						}
					}
				}
				else
					client_socket->readBuffer.clear();
			}
        }
    }

    // Cleanup
    for (std::list<t_socket>::iterator it = listenSockets.begin(); it != listenSockets.end(); ++it)
        close(it->socket_fd);

    for (std::list<t_socket>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it)
        close(it->socket_fd);

    close(epoll_fd);
}