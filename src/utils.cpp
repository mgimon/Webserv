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
    HttpResponse http_response;
    std::map<std::string, std::string>  responseHeaders;
    http_response.setVersion("HTTP/1.1");

    std::string content_type;
    std::string path = http_request.getPath();
    if (path == "/" || path.empty())
    {
        path = "index.html";
        content_type = "text/html";
    }
    path = "var/www/html/" + path;

    std::ifstream file(path.c_str());
    if (!file)
    {
        std::cerr << "Could not open " << path << "\n";

        http_response.setStatusCode(500);
        http_response.setStatusMessage("Internal Server Error :(");
        responseHeaders.insert(std::make_pair("Content-Length", "0"));
        responseHeaders.insert(std::make_pair("Connection", "close"));
        http_response.setHeaders(responseHeaders);

        write(client_fd, http_response.buildResponse().c_str(), http_response.buildResponse().size());
        return (1);
    }

    // we could open the file
    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::ostringstream oss;
    oss << body.size();

    http_response.setStatusCode(200);
    http_response.setStatusMessage("OK");
    responseHeaders.insert(std::make_pair("Content-Type", ""));
    responseHeaders.insert(std::make_pair("Content-Length", oss.str()));
    responseHeaders.insert(std::make_pair("Connection", "keep-alive"));
    http_response.setHeaders(responseHeaders);

    http_response.setBody(body);

    write(client_fd, http_response.buildResponse().c_str(), http_response.buildResponse().size());
    return (0);
}


}
