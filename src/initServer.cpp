#include "../include/initServer.hpp"

std::vector<t_listen> getValidListens(const std::vector<ServerConfig> &serverList)
{
	std::vector<t_listen> valid_listens;
	std::map<std::pair<int, std::string>, t_listen> map_l;

	//Añadimos todos los listens a un map, si ya existe uno igual el map no lo añadira
	for (std::vector<ServerConfig>::const_iterator it = serverList.begin(); it != serverList.end(); ++it)
	{
		std::vector<t_listen> listens = it->getListens();
		for (std::vector<t_listen>::iterator listen_it = listens.begin(); listen_it != listens.end(); ++listen_it)
		{
			std::pair<int, std::string> id = std::make_pair(listen_it->port, listen_it->ip);
			map_l.insert(std::make_pair(id, *listen_it));
		}
	}

	//Guardamos los listens validos. Si hay uno con ip 0.0.0.0, sera el unico de ese puerto que se guarde
	int actual_port = -1;
	bool wildcard = false;
	for (std::map<std::pair<int, std::string>, t_listen>::iterator it = map_l.begin(); it != map_l.end(); ++it)
	{
		if (wildcard)
		{
			if (it->first.first != actual_port)
			{
				valid_listens.push_back(it->second);
				wildcard = false;
			}
		}
		else
		{
			if (it->first.second == "0.0.0.0")
			{
				actual_port = it->first.first; 
				wildcard = true;
			}
			valid_listens.push_back(it->second);
		}
	}
	return(valid_listens);
}

/*
addrinfo *getAddrinfoList(t_listen listen)
{

}
int createListenSocket(t_listen listen_conf)
{
	int socket_fd;
	addrinfo *addrinfo_list = getAddrinfoList(listen_conf); // Obtenemos una lista de configuraciones posibles para el socket
	addrinfo *node = addrinfo_list;
	for (addrinfo *node = addrinfo_list; node != NULL; node = node->ai_next)
	{
		socket_fd = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
		if (socket_fd == -1)
			continue;
		if (bind(socket_fd, node->ai_addr, node->ai_addrlen) == 0) // Assignamos la configuracion de addr al socket
			break;
		close(socket_fd);
	}
	freeaddrinfo(addrinfo_list);
	if (node == NULL)
		throw std::runtime_error("Error binding socket");
	//Listen
	if (listen(socket_fd, listen_conf.backlog) == -1)
		throw std::runtime_error("Error putting socket to listen");
	return(socket_fd);
}
*/

std::vector<int> loadListenSockets(const std::vector<ServerConfig> &serverList)
{
	std::vector<int> listenSockets;
	std::vector<t_listen> valid_listens = getValidListens(serverList);
	/*for(std::vector<t_listen>::iterator it = valid_listens.begin(); it != valid_listens.end(); ++it)
	{
		int socket_fd = createListenSocket(*it);
		listenSockets.push_back(socket_fd);
	}*/
	for (std::vector<t_listen>::iterator it = valid_listens.begin(); it != valid_listens.end(); ++it)
		std::cout << "Listen " << it->ip << ":" << it->port  << " backlog: " << it->backlog << std::endl;
	return(listenSockets);
}


void initServer(std::vector<ServerConfig> &serverList)
{
	std::vector<int> listenSockets = loadListenSockets(serverList);
}