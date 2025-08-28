#include "../include/ServerConfig.hpp"
#include "../include/HttpRequest.hpp"
#include "../include/HttpResponse.hpp"
#include "../include/utils.hpp"

int status = 0;

// TODO check keep alive vs status return
int main() {

    /*** PARSEO ***/
        // hardcodear atributos de objeto serverList para poder ir trabajando
    std::vector<ServerConfig>   serverList;

    /*** GESTION DE CONEXIONES ***/
        // multiples clientes (poll/epoll ?)
        // multiples servidores (procesos ?)

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int client_fd;
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1); // backlog

    /***  RESPUESTA ***/
    // provisional para 1 cliente
    client_fd = accept(server_fd, NULL, NULL);
    bool    keep_alive = true;
    while (keep_alive)
    {
        HttpRequest http_request(client_fd);
        http_request.printRequest();
        status = utils::respond(client_fd, http_request, keep_alive);
        if (!keep_alive)
            break;
    }
    close(client_fd);
    close(server_fd);

    return status;
}
