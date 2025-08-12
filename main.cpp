#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

int main() {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    int client_fd = accept(server_fd, nullptr, nullptr);

    char buffer[4096];
    ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0'; // C-string termina aquí

        // Construir respuesta HTTP con el contenido recibido
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(bytes) + "\r\n"
            "\r\n" +
            std::string(buffer, bytes);

        write(client_fd, response.c_str(), response.size());
    }

    close(client_fd);
    close(server_fd);
    return 0;
}