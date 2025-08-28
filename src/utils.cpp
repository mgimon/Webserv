#include "../include/utils.hpp"

namespace utils {

void validatePath(std::string &path)
{
    if (path.empty() || path == "/")
        path = "index.html";
    else if (path[0] == '/')
        path.erase(0, 1);  // quitar '/'

    path = "var/www/html/" + path;
}

// Debe incluir gestion de CGI
int respond(int client_fd, const HttpRequest &http_request, bool &keep_alive) {
    const std::string &method = http_request.getMethod();

    if (method == "GET") {
        std::cout << "Method get" << std::endl;
        return respondGet(client_fd, http_request, keep_alive);
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
        return (1);
    }
}

int respondGet(int client_fd, const HttpRequest &http_request, bool &keep_alive)
{
    HttpResponse respondTool;
    std::string path = http_request.getPath();

    if (path.empty() || path == "/")
        path = "index.html";
    else if (path[0] == '/')
        path.erase(0, 1);

    path = "var/www/html/" + path;
    std::string response = respondTool.buildResponse(path);

    // headers del response
    std::map<std::string, std::string> headers = respondTool.getHeaders();

    std::map<std::string, std::string>::const_iterator it = http_request.getHeaders().find("Connection");
    if (it != http_request.getHeaders().end() && it->second == "keep-alive") {
        headers["Connection"] = "keep-alive";
        keep_alive = true;
    } else if (http_request.getVersion() == "HTTP/1.1") {
        headers["Connection"] = "keep-alive";
        keep_alive = true;
    } else {
        headers["Connection"] = "close";
        keep_alive = false;
    }

    std::ostringstream final_response;
    final_response << respondTool.getVersion() << " " << respondTool.getStatusCode()
                   << " " << respondTool.getStatusMessage() << "\r\n";
    for (std::map<std::string, std::string>::iterator h = headers.begin(); h != headers.end(); ++h)
        final_response << h->first << ": " << h->second << "\r\n";
    final_response << "\r\n" << respondTool.getBody();

    write(client_fd, final_response.str().c_str(), final_response.str().size());
    return 0;
}



}
