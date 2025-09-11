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

void handleKeepAlive(const HttpRequest &http_request, bool &keep_alive, HttpResponse &respondTool)
{
    std::map<std::string, std::string> headers = respondTool.getHeaders();
    std::map<std::string, std::string>::const_iterator it = http_request.getHeaders().find("Connection");
    if (it != http_request.getHeaders().end() && it->second == "keep-alive")
    {
        headers["Connection"] = "keep-alive";
        keep_alive = true;
    }
    else if (http_request.getVersion() == "HTTP/1.1")
    {
        headers["Connection"] = "keep-alive";
        keep_alive = true;
    }
    else
    {
        headers["Connection"] = "close";
        keep_alive = false;
    }
    respondTool.setHeaders(headers);
}

// Debe incluir gestion de CGI
int respond(int client_fd, const HttpRequest &http_request, ServerConfig &serverOne, bool &keep_alive)
{
    HttpResponse http_response;
    const std::string &method = http_request.getMethod();
    const LocationConfig *requestLocation = locationMatchforRequest(http_request.getPath(), serverOne.getLocations());

    printLocation(requestLocation);

    if (method == "GET")
    {
        std::cout << "Method get" << std::endl;
        if (isMethodAllowed(requestLocation->getMethods(), "GET"))
            return respondGet(client_fd, http_request, http_response, keep_alive);
        else
        {
            http_response.setError("var/www/html/405MethodNotAllowed.html", 405, "Method Not Allowed");
            http_response.respondInClient(client_fd);
            return (1);
        }
    }
    else if (method == "POST")
    {
        std::cout << "Method post" << std::endl;
        if (isMethodAllowed(requestLocation->getMethods(), "POST"))
            return respondPost(client_fd, http_request, http_response, keep_alive);
        else
        {
            http_response.setError("var/www/html/405MethodNotAllowed.html", 405, "Method Not Allowed");
            http_response.respondInClient(client_fd);
            return (1);
        }
    }
    else if (method == "DELETE") {
        // manejar DELETE
        std::cout << "Method delete" << std::endl;
        return (0);
    }
    else {
        // metodo no soportado - 405 Method Not Allowed
        std::cout << "Other method" << std::endl;
        return (1);
    }
}

int respondGet(int client_fd, const HttpRequest &http_request, HttpResponse &http_response, bool &keep_alive)
{
    std::string path = http_request.getPath();
    validatePathWithIndex(path);

    http_response.buildResponse(path);
    handleKeepAlive(http_request, keep_alive, http_response);

    http_response.respondInClient(client_fd);
    return (0);
}

int respondPost(int client_fd, const HttpRequest &http_request, HttpResponse &http_response, bool &keep_alive)
{
    (void)client_fd;
    (void)http_request;
    (void)http_response;
    (void)keep_alive;
    
    return 0;
}

void hardcodeMultipleLocServer(ServerConfig &server)
{

    server.setHost("0.0.0.0");
    server.setPort(8080);
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
    //upload_methods.push_back("POST");
    loc_upload.setMethods(upload_methods);
    loc_upload.setAutoIndex(false);

    // Add locations to server object
    std::vector<LocationConfig> locations;
    locations.push_back(loc_root);
    locations.push_back(loc_images);
    locations.push_back(loc_upload);
    server.setLocations(locations);

}

const LocationConfig* locationMatchforRequest(const std::string &request_path, const std::vector<LocationConfig> &locations)
{
    const LocationConfig* best_match = NULL;
    size_t best_len = 0;

    for (size_t i = 0; i < locations.size(); i++)
    {
        const std::string &loc_path = locations[i].getPath();
        if (request_path.find(loc_path) == 0) {
            if (loc_path.size() > best_len) {
                best_len = loc_path.size();
                best_match = &locations[i];
            }
        }
    }

    return (best_match); // puede ser NULL si no hay match → usar defaults
}



}
