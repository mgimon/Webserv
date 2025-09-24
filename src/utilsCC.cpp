#include "../include/utilsCC.hpp"

std::string to_stringCC(int num)
{
	std::stringstream ss;
    ss << num;
	
	return(ss.str());
}

void closeListenSockets(std::list<t_socket> &listenSockets)
{
    for (std::list<t_socket>::iterator it = listenSockets.begin(); it != listenSockets.end(); ++it)
        close(it->socket_fd);
}

void closeServer(int epoll_fd, std::map<int, t_socket> &clientSockets, std::list<t_socket> &listenSockets)
{
	closeListenSockets(listenSockets); // NOTA: CONVERTIR FUNCTION EN TEMPLATE
	for (std::map<int, t_socket>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it)
		close(it->second.socket_fd);
	close(epoll_fd);
}

/*std::vector<t_listen> getValidListens(const std::vector<ServerConfig> &serverList)
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
*/