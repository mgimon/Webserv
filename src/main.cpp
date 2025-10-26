#include "../include/ConfigParser.hpp"
#include "../include/initServer.hpp"
#include "../include/utils.hpp"
#include <iostream>


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return EXIT_FAILURE;
    }
    std::string configFile = argv[1];
    ConfigParser parser;
    try {
        parser.parse(configFile);
        std::vector<ServerConfig> serverList = parser.getServers();
        initServer(serverList);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
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
