#include "../include/utils.hpp"

namespace utils {

// Debe incluir gestion de CGI
int respond(int client_fd, int server_fd, const HttpRequest &http_request, bool &keep_alive) {
    const std::string &method = http_request.getMethod();

    if (method == "GET") {
        return respondGet(client_fd, server_fd, http_request, keep_alive);
    }
    else if (method == "POST") {
        // manejar POST
        return 0;
    }
    else if (method == "DELETE") {
        // manejar DELETE
        return 0;
    }
    else {
        // metodo no soportado - 405 Method Not Allowed
        std::string response =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";
        write(client_fd, response.c_str(), response.size());
        return 1;
    }
}

int respondGet(int client_fd, int server_fd, const HttpRequest &http_request, bool &keep_alive)
{
    (void)server_fd;

    std::map<std::string, std::string> headers = http_request.getHeaders();
    std::map<std::string, std::string>::iterator it = headers.find("Connection");
    if (it != headers.end() && it->second == "close")
        keep_alive = false;

    std::string full_path;
    std::string content_type;
    std::string path = http_request.getPath();

    // que hay escrito en el fd de la peticion?
    // GET /css/styles.css HTTP/1.1
    // GET /js/app.js HTTP/1.1
    // GET /images/logo.png HTTP/1.1
    // crear objeto HttpRespond, con path a servir y content type
    if (path == "/")
    {
        full_path = "var/www/html/index.html";
        content_type = "text/html";
    }
    else if (path == "/css/styles.css")
    {
        full_path = "var/www/html/css/styles.css";
        content_type = "text/css";
    }
    else
    {
        std::string response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";
        write(client_fd, response.c_str(), response.size());
        return 1;
    }

    std::ifstream file(full_path.c_str());
    if (!file)
    {
        std::cerr << "Could not open " << full_path << "\n";
        std::string response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";
        write(client_fd, response.c_str(), response.size());
        return 1;
    }

    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::ostringstream oss;
    oss << body.size();

    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + oss.str() + "\r\n"
        "Connection: " + (keep_alive ? "keep-alive" : "close") + "\r\n"
        "\r\n" +
        body;

    write(client_fd, response.c_str(), response.size());

    return 0;
}


}
