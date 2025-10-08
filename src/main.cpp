#include "../include/ServerConfig.hpp"
#include "../include/initServer.hpp"
#include "../include/utils.hpp"

int main() {
	
	std::vector<ServerConfig> serverList;
	ServerConfig server;
	ServerConfig server2;

	//server.addListen((t_listen){ "127.0.0.1", 8080, 128});
	utils::hardcodeMultipleLocServer(server);

	serverList.push_back(server);

	
	try
	{
		initServer(serverList);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return(EXIT_FAILURE);
	}

	return(EXIT_SUCCESS);

}