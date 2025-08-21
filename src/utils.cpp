#include "../include/utils.hpp"

namespace utils {

// Debe incluir gestion de metodos GET, POST, DELETE y gestion de CGI!
int respond(int client_fd, int server_fd, bool &keep_alive) {

	(void)server_fd;
    while (keep_alive)
    {
        char buffer[1024];
        int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            close(client_fd);
            return (1);
        }
        buffer[bytes_read] = '\0';

        std::string request(buffer);


        std::string first_line = request.substr(0, request.find("\r\n"));
        std::string path = first_line.substr(4, first_line.find(" ", 4) - 4);

        if (request.find("Connection: close") != std::string::npos) {
            keep_alive = false;
        }

        std::string full_path;
        std::string content_type;

        if (path == "/") {
            full_path = "var/www/html/index.html";
            content_type = "text/html";
        } else if (path == "/css/styles.css") {
            full_path = "var/www/html/css/styles.css";
            content_type = "text/css";
        } else {
            std::string response =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 0\r\n"
                "\r\n";
            write(client_fd, response.c_str(), response.size());
            if (!keep_alive) close(client_fd);
            	return (1);
        }

        std::ifstream file(full_path.c_str());
        if (!file) {
            std::cerr << "Could not open " << full_path << "\n";
            if (!keep_alive) close(client_fd);
            	return (1);
        }

        std::string body((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

        std::ostringstream oss;
        oss << body.size();

        // Enviar respuesta con Connection correcta
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: " + content_type + "\r\n"
            "Content-Length: " + oss.str() + "\r\n"
            "Connection: " + (keep_alive ? "keep-alive" : "close") + "\r\n"
            "\r\n" +
            body;

        write(client_fd, response.c_str(), response.size());

        // Si el cliente pidio cerrar, salimos del loop
        if (!keep_alive)
            break;
    }

    close(client_fd);
    return (0);
}

}
