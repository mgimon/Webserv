#include "../include/ServerConfig.hpp"
#include "../include/initServer.hpp"

int main() {
	
	std::vector<ServerConfig> serverList;
	ServerConfig server;
	ServerConfig server2;

	server.addListen((t_listen){ "0.0.0.0", 433, 128});
	server2.addListen((t_listen){ "127.0.0.1", 80, 128});
	serverList.push_back(server2);
	serverList.push_back(server);

	try
	{
		initServer(serverList);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	

}