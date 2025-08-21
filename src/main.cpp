#include "../include/ServerConfig.hpp"
#include "../include/utils.hpp"

int status = 0;

int main() {

    /*** PARSEO ***/
        // hardcodear atributos de objeto serverList para poder ir trabajando
    std::vector<ServerConfig>   serverList;

    /*** GESTION DE CONEXIONES ***/
        // multiples clientes (poll/epoll ?)
        // multiples servidores (procesos ?)

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1); // backlog

    bool    keep_alive = true;
    while (keep_alive)
    {
        int client_fd = accept(server_fd, NULL, NULL);
        /***  RESPUESTA ***/ status = utils::respond(client_fd, server_fd, keep_alive);
        if (!keep_alive)
            close(client_fd);
    }

    close(server_fd);

    return status;
}
