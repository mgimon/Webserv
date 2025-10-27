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