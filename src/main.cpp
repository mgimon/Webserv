#include "../include/ConfigParser.hpp"
#include "../include/initServer.hpp"
#include "../include/utils.hpp"
#include <iostream>


int main() {
    ConfigParser parser;
    try {
        parser.parse("default.conf"); // Remplace par le nom de ton fichier de config
	std::vector<ServerConfig> serverList = parser.getServers();
	initServer(serverList);
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
/*
int main() {
    std::vector<ServerConfig> serverList;
    ServerConfig server;
    ServerConfig server2;
    //server.addListen((t_listen){ "127.0.0.1", 8080, 128});
    utils::hardcodeMultipleLocServer(server);
    serverList.push_back(server);
    server.print();
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
}*/
