#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../include/ServerConfig.hpp"

int main() {

    // PARSEO
    std::vector<ServerConfig>   serverList;
        // hardcodear atributos de objeto serverList para poder ir trabajando

    // GESTION DE CONEXIONES
        // incluye gestion de multiples clientes (poll/epoll)
        // incluye gestion de multiples procesos? (servidores)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1); // backlog

    int client_fd = accept(server_fd, NULL, NULL);

    // RESPUESTA
        // incluye gestion de metodos (GET, POST, DELETE)
        // incluye gestion de CGI
    std::ifstream file("var/www/html/index.html");
    if (!file) {
        std::cerr << "Could not open index.html\n";
        close(client_fd);
        close(server_fd);
        return 1;
    }
    std::string body((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" +
        body;

    write(client_fd, response.c_str(), response.size());

    close(client_fd); // provisional para 1 conexion
    close(server_fd); // provisional para 1 conexion

    return 0;
}
