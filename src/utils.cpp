#include "../include/utils.hpp"

namespace utils {

void printLocation(const LocationConfig *location)
{
    if (location) {
    const std::vector<std::string> &methods = location->getMethods();
    std::string methods_str = methods.empty() ? "none" : methods[0];
    for (size_t i = 1; i < methods.size(); ++i)
        methods_str += ", " + methods[i];

    std::cout << YELLOW
              << "Matched Location -> Path: " << location->getPath()
              << " | Methods: " << methods_str
              << " | AutoIndex: " << (location->getAutoIndex() ? "true" : "false")
              << RESET << std::endl;
    } else {
        std::cout << YELLOW << "No matching location found." << RESET << std::endl;
    }
}

bool isMethodAllowed(const std::vector<std::string> &methods, const std::string &method)
{
    for (size_t i = 0; i < methods.size(); ++i)
    {
        if (method == methods[i])
            return (true);
    }
    return (false);
}

void validatePathWithIndex(std::string &path)
{
    if (path.empty() || path == "/")
        path = "index.html";
    else if (path[0] == '/')
        path.erase(0, 1);  // quitar '/'

    path = "var/www/html/" + path;
}

std::string getErrorPath(ServerConfig &serverOne, int errcode)
{
        std::string errorpath = serverOne.getDocumentRoot() + "/" + serverOne.getErrorPageName(errcode);
        if (!errorpath.empty() && errorpath[0] == '/')
            errorpath = errorpath.substr(1);
        
        return (errorpath);
}

// Debe incluir gestion de CGI
int respond(int client_fd, const HttpRequest &http_request, ServerConfig &serverOne)
{
    HttpResponse    http_response;
    const std::string &method = http_request.getMethod();

    const LocationConfig *requestLocation = locationMatchforRequest(http_request.getPath(), serverOne.getLocations());

    printLocation(requestLocation);

    if (method == "GET")
    {
        std::cout << "Method get" << std::endl;
        if (requestLocation && isMethodAllowed(requestLocation->getMethods(), "GET"))
        {
            std::string path = http_request.getPath();
            utils::validatePathWithIndex(path);
            return respondGet(serverOne, client_fd, path, http_request, http_response);
        }
        else
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            http_response.respondInClient(client_fd);
            return (1);
        }
    }
    else if (method == "POST")
    {
        if (requestLocation && isMethodAllowed(requestLocation->getMethods(), "POST"))
        {
            std::string response =
            "HTTP/1.1 303 See Other\r\n"
            "Location: /form_result\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";

            send(client_fd, response.c_str(), response.size(), 0);
            http_response.respondInClient(client_fd);
            return (0);
        }
        else
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            http_response.respondInClient(client_fd);
            return (1);
        }
    }
    else if (method == "DELETE") {
        std::cout << "Method delete" << std::endl;
        return (0);
    }
    else {
        std::cout << "Other method" << std::endl;
        http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
        http_response.respondInClient(client_fd);
        return (1);
    }
}

int respondGet(ServerConfig &serverOne, int client_fd, std::string path, const HttpRequest &http_request, HttpResponse &http_response)
{
    int keep_alive = 0;

    http_response.buildResponse(path, serverOne);

    std::map<std::string, std::string> headers = http_response.getHeaders();
    std::map<std::string, std::string>::const_iterator it = http_request.getHeaders().find("Connection");
    if (it != http_request.getHeaders().end() && it->second == "close")
    {
        headers["Connection"] = "close";
        keep_alive = -1;
    }
    else
        headers["Connection"] = "keep-alive";
    
    http_response.setHeaders(headers);
    http_response.respondInClient(client_fd);

    return (keep_alive);
}

int respondPost(int client_fd, const HttpRequest &http_request, HttpResponse &http_response)
{
    (void)client_fd;
    (void)http_request;
    (void)http_response;
    
    return (0);
}

bool isCompleteRequest(const std::string& str)
{
    size_t headersEnd = str.find("\r\n\r\n");
    if (headersEnd == std::string::npos)
        return (false); 

    size_t bodyStart = headersEnd + 4;
    size_t contentLength = 0;

    std::istringstream stream(str.substr(0, headersEnd));
    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1, 1);

        if (line.find("Content-Length:") == 0)
        {
            contentLength = std::atoi(line.substr(15).c_str());
            break;
        }
    }

    if (str.size() >= bodyStart + contentLength)
        return (true);

    return (false);
}

void readFromSocket(t_socket *client_socket, int epoll_fd, std::map<int, t_socket> &clientSockets)
{
    char buf[4096];
    ssize_t bytesRead = recv(client_socket->socket_fd, buf, sizeof(buf), 0);

    if (bytesRead <= 0)
    {
        // cliente cerro la conexion o error -> cerrar socket
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket->socket_fd, NULL);
        close(client_socket->socket_fd);
        clientSockets.erase(client_socket->socket_fd);
        return;
    }
    // leido -> append
    client_socket->readBuffer.append(buf, bytesRead);
}

void hardcodeMultipleLocServer(ServerConfig &server)
{
    //server.setHost("0.0.0.0");
    //server.setPort(8080);

    t_listen listenOne;
    listenOne.host = "0.0.0.0";
    listenOne.port = 8080;
    listenOne.backlog = 128;

    server.addListen(listenOne);
    server.setDocumentRoot("/var/www/html");

    // Location "/"
    LocationConfig loc_root;
    loc_root.setPath("/");
    std::vector<std::string> root_methods;
    root_methods.push_back("GET");
    //root_methods.push_back("POST");
    loc_root.setMethods(root_methods);
    loc_root.setAutoIndex(false);

    // Location "/images/"
    LocationConfig loc_images;
    loc_images.setPath("/images/");
    std::vector<std::string> images_methods;
    images_methods.push_back("GET");
    loc_images.setMethods(images_methods);
    loc_images.setAutoIndex(true);

    // Location "/upload/"
    LocationConfig loc_upload;
    loc_upload.setPath("/upload/");
    std::vector<std::string> upload_methods;
    upload_methods.push_back("POST");
    loc_upload.setMethods(upload_methods);
    loc_upload.setAutoIndex(false);

    // Location "/form_result/"
    LocationConfig loc_form;
    loc_form.setPath("/form_result/");
    std::vector<std::string> form_methods;
    form_methods.push_back("POST");
    form_methods.push_back("GET");
    loc_form.setMethods(form_methods);
    loc_form.setAutoIndex(false);

    // Add locations to server object
    std::vector<LocationConfig> locations;
    locations.push_back(loc_root);
    locations.push_back(loc_images);
    locations.push_back(loc_upload);
    locations.push_back(loc_form);
    server.setLocations(locations);

}

std::string normalizePathForMatch(const std::string &path) {
    if (path.empty()) return "/";
    if (path[path.size() - 1] != '/')
        return (path + "/");
    return path;
}

const LocationConfig* locationMatchforRequest(const std::string &request_path, const std::vector<LocationConfig> &locations)
{
    std::string req = normalizePathForMatch(request_path);
    const LocationConfig* best_match = NULL;
    size_t best_len = 0;

    for (size_t i = 0; i < locations.size(); i++)
    {
        std::string loc_path = normalizePathForMatch(locations[i].getPath());
        if (req.find(loc_path) == 0 && loc_path.size() > best_len)
        {
            best_len = loc_path.size();
            best_match = &locations[i];
        }
    }
    return (best_match);
}




}
