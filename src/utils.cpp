#include "../include/utils.hpp"

namespace utils {

// Debe incluir gestion de CGI
// 
int respond(int client_fd, int server_fd, const HttpRequest &http_request, bool &keep_alive) {
    const std::string &method = http_request.getMethod();

    if (method == "GET") {
        std::cout << "Method get" << std::endl;
        return respondGet(client_fd, server_fd, http_request, keep_alive);
    }
    else if (method == "POST") {
        // manejar POST
        std::cout << "Method post" << std::endl;
        return (0);
    }
    else if (method == "DELETE") {
        // manejar DELETE
        std::cout << "Method delete" << std::endl;
        return (0);
    }
    else {
        // metodo no soportado - 405 Method Not Allowed
        std::cout << "Other method" << std::endl;
        http_request.printRequest();
        /*std::string response =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";
        write(client_fd, response.c_str(), response.size());*/
        return (1);
    }
}

int respondGet(int client_fd, int server_fd, const HttpRequest &http_request, bool &keep_alive)
{
    // que hay escrito en el fd de la peticion?
    // GET /css/styles.css HTTP/1.1
    // GET /js/app.js HTTP/1.1
    // GET /images/logo.png HTTP/1.1

    (void)server_fd;
    (void)keep_alive;
    HttpResponse respondTool;
    std::string response;

    std::string content_type;
    std::string path = http_request.getPath();

    if (path.empty() || path == "/")
        path = "index.html";
    else if (path[0] == '/')
        path.erase(0, 1);  // quitar '/' inicial

    path = "var/www/html/" + path;

    response = respondTool.buildResponse(path);
    write(client_fd, response.c_str(), response.size());
    return (0);
}


}
