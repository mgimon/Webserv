#include "../include/ConfigParser.hpp"
#include "../include/initServer.hpp"
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
