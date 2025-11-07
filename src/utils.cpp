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

std::string getFirstValidFile(std::vector<std::string> files, const std::string &root)
{
    for (std::vector<std::string>::iterator it = files.begin(); it != files.end(); ++it)
    {
        std::ifstream file((root + "/" + *it).c_str());
        //std::cout << PINK << "Trying " << (root + "/" + *it) << RESET << std::endl;
        if (file.good())
            return (*it);
    }

    return ("generic_error");
}

bool isFile(std::string &path)
{
    if (path.empty())
        return false;
    size_t lastSlash = path.find_last_of('/');
    size_t lastDot = path.find_last_of('.');
    if (lastDot != std::string::npos && (lastSlash == std::string::npos || lastDot > lastSlash))
        return (true);
    if (path[path.size() - 1] != '/')
        path += '/';
    return (false);
}

// evals "/" as true
int isLocation(ServerConfig &serverOne, std::string &path)
{
    std::vector<LocationConfig> locations = serverOne.getLocations();
    std::string locationPath;

    if (path == "/")
        return (0);
    for (std::vector<LocationConfig>::iterator it = locations.begin(); it != locations.end(); ++it)
    {
        locationPath = it->getPath();
        if (!locationPath.empty() && locationPath[locationPath.size() - 1] == '/')
            locationPath = locationPath.substr(0, locationPath.size() - 1);
        if (locationPath.empty())
            continue;
        if (path == locationPath || (path.find(locationPath + "/") == 0))
            return (1);
    }
    return (-1);
}

void trimPathSlash(std::string &path)
{
    if (!path.empty() && path[0] == '/' && path != "/")
        path = path.substr(1);
}

bool isRawLocationRequest(ServerConfig &serverOne, std::string &path)
{
    std::vector<LocationConfig> locations = serverOne.getLocations();
    std::string locationPath;

    for (std::vector<LocationConfig>::iterator it = locations.begin(); it != locations.end(); ++it)
    {
        locationPath = it->getPath();
        if (!locationPath.empty() && locationPath[locationPath.size() - 1] == '/')
            locationPath = locationPath.substr(0, locationPath.size() - 1);
        if (path == locationPath)
            return (true);
    }
    return (false);
}


void validatePathWithIndex(std::string &path, const LocationConfig *requestLocation, ServerConfig &serverOne)
{
    if (path.empty() || path == "/" || !isFile(path))
    {
        std::string indexToServe;
        if (requestLocation->getLocationIndexFiles().empty())
            indexToServe = serverOne.getDefaultFile();
        else
        {
            std::vector<std::string> indexfiles = requestLocation->getLocationIndexFiles();
            std::cout << RED <<
            "Printing files: ";
            for (std::vector<std::string>::size_type i = 0; i < indexfiles.size(); ++i)
                std::cout << indexfiles[i] << " ";
            std::cout << RESET << std::endl;
            std::string root;
            if (!requestLocation->getRootOverride().empty())
                root = requestLocation->getRootOverride();
            else
                root = serverOne.getDocumentRoot();
            indexToServe = getFirstValidFile(requestLocation->getLocationIndexFiles(), root);
            std::cout << RED << "Indextoserve is: " << indexToServe << RESET << std::endl;
        }
        
        path = indexToServe;

    }
    else if (path[0] == '/')
        path.erase(0, 1);  // quitar '/'
    if (!requestLocation->getRootOverride().empty())
        path = requestLocation->getRootOverride() + '/' + path;
    else
        path = serverOne.getDocumentRoot() + '/' + path;
    if (path[0] == '/')
        path = path.substr(1);

    //std::cout << YELLOW << "Path is " << path << RESET << std::endl;
}

std::string getErrorPath(ServerConfig &serverOne, int errcode)
{
    std::string errorpath = serverOne.getDocumentRoot() + "/" + serverOne.getErrorPageName(errcode);
    if (!errorpath.empty() && errorpath[0] == '/')
        errorpath = errorpath.substr(1);

    std::cout << RED << "Error path is " << errorpath << RESET << std::endl;
    
    return (errorpath);
}

int serveRedirect(const LocationConfig *requestLocation, int client_fd, HttpResponse &http_response)
{
    if (requestLocation)
    {
        std::pair<int, std::string> redirect = requestLocation->getRedirect();
        std::cout << YELLOW << "Redirect int: " << redirect.first << RESET << std::endl;
        std::cout << YELLOW << "Redirect path: " << redirect.second << RESET << std::endl;
        if (redirect.first != 0)
        {
                http_response.setRedirectResponse(redirect.first, redirect.second);
                http_response.respondInClient(client_fd);
                std::cout << YELLOW << "Redirect served!" << RESET << std::endl;
                return (1);
        }
    }
    return (-1);
}

int serveGet(const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{
    if (!requestLocation)
    {
        http_response.setError(getErrorPath(serverOne, 404), 404, "Not Found");
        http_response.respondInClient(client_fd);
        return (1);
    }
    std::string path = http_request.getPath();
    //std::cout << CYAN << "Path " << path << " is location?: " << std::boolalpha << isLocation(serverOne, path) << RESET << std::endl;
    //http_response.setResponse(200, "200 OK");
    //http_response.respondInClient(client_fd);

    // asking for raw root (serves first valid index from vector)
    if (isLocation(serverOne, path) == 0)
    {
        validatePathWithIndex(path, requestLocation, serverOne);
        if (isMethodAllowed(requestLocation->getMethods(), "GET"))
            return respondGet(serverOne, client_fd, path, http_request, http_response);
        else
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            http_response.respondInClient(client_fd);
            return (1);
        }
    }
    // inside of root
    else if (isLocation(serverOne, path) == -1)
    {
        if (isDirectory(serverOne.getDocumentRoot() + path))
        {
            if (requestLocation->getAutoIndex() == false)
            {
                http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
                http_response.respondInClient(client_fd);
                return (1);
            }
            else
            {
                std::string autoindex_page = utils::generateAutoindex(serverOne.getDocumentRoot() + path);
                http_response.setResponse(200, autoindex_page);
                http_response.respondInClient(client_fd);
                return (0);
            }
        }
        if (isMethodAllowed(requestLocation->getMethods(), "GET"))
            return respondGet(serverOne, client_fd, serverOne.getDocumentRoot() + path, http_request, http_response);
        else
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            http_response.respondInClient(client_fd);
            return (1);
        }
    }
    // inside location
    else if (isLocation(serverOne, path) == 1)
    {
        std::cout << PINK << path << RESET << std::endl;
        std::string tempPath = path;
        trimPathSlash(tempPath);
        if (isDirectory(tempPath) && !isRawLocationRequest(serverOne, path))
        {
            if (requestLocation->getAutoIndex() == false)
            {
                http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
                http_response.respondInClient(client_fd);
                return (1);
            }
            else
            {
                std::string autoindex_page = generateAutoindex(path);
                http_response.setResponse(200, autoindex_page);
                http_response.respondInClient(client_fd);
                return (0);
            }
        }
        if (isRawLocationRequest(serverOne, path) && requestLocation->getAutoIndex() == true)
        {
            std::string autoindex_page = generateAutoindex(path);
            http_response.setResponse(200, autoindex_page);
            http_response.respondInClient(client_fd);
            return (0);
        }
        if (!requestLocation->getRootOverride().empty() && requestLocation->getLocationIndexFiles().empty())
        {
            http_response.setError(getErrorPath(serverOne, 404), 404, "Not Found");
            http_response.respondInClient(client_fd);
            return (1);
        }
        std::cout << PINK << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << RESET << std::endl;
        std::cout << PINK << requestLocation->getRootOverride() + path << RESET << std::endl;
        if (isMethodAllowed(requestLocation->getMethods(), "GET"))
        {
            if (isRawLocationRequest(serverOne, path))
            {
                if (requestLocation->getLocationIndexFiles().empty())
                    return respondGet(serverOne, client_fd, serverOne.getDocumentRoot() + "/" + getFirstValidFile(serverOne.getServerIndexFiles(), serverOne.getDocumentRoot()), http_request, http_response);
                else
                    return respondGet(serverOne, client_fd, requestLocation->getRootOverride() + "/" + getFirstValidFile(requestLocation->getLocationIndexFiles(), requestLocation->getRootOverride()), http_request, http_response);
            }
            else
                return respondGet(serverOne, client_fd, requestLocation->getRootOverride() + "/" + path, http_request, http_response);
        }
        else
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            http_response.respondInClient(client_fd);
            return (1);
        }
    }
    return (0);
    
}

std::string getFormSuccessBody()
{
    std::string html =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "    <meta charset=\"UTF-8\" />\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n"
    "    <title>Form sent</title>\n"
    "    <link rel=\"stylesheet\" href=\"/styles.css\" />\n"
    "</head>\n"
    "<body>\n"
    "    <header><h1>Thanks for submitting!!</h1></header>\n"
    "    <main>\n"
    "        <div class=\"maindiv_first\">\n"
    "            <p>The form has been sent</p>\n"
    "        </div>\n"
    "    </main>\n"
    "    <footer><p>© 2025 Http Enjoyers</p></footer>\n"
    "</body>\n"
    "</html>\n";

    return (html);
}

bool isUpload(const HttpRequest &http_request)
{
    std::map<std::string, std::string> headers = http_request.getHeaders();
    std::map<std::string, std::string>::const_iterator it = headers.find("Content-Type");
    if (it == headers.end())
        return (false);
    const std::string value = it->second;
    if (value.find("multipart/form-data") == std::string::npos)
        return (false);
    return (true);
}

int isStorageAllowed(ServerConfig &serverOne)
{
    std::vector<LocationConfig> locations = serverOne.getLocations();
    for (size_t i = 0; i < locations.size(); i++)
    {
        if (locations[i].getPath() == "/upload/")
        {
            if (isMethodAllowed(locations[i].getMethods(), "POST"))
                return (0);
            return (405);
        }
    }
    return (403);
}

std::string getUploadFilename(const HttpRequest &http_request)
{
    std::string body = http_request.getBody();
    std::string key = "filename=\"";

    size_t start = body.find(key);
    if (start == std::string::npos)
        return ("file.txt");
    start += key.length();
    size_t end = body.find("\"", start);
    if (end == std::string::npos)
        return ("file.txt");
    return (body.substr(start, end - start));
}

void trimWebKitFormBoundary(std::string &body)
{
    std::string fullLine;
    std::string key = "------WebKitFormBoundary";

    size_t start = body.find("\n\n");
    if (start != std::string::npos)
        body = body.substr(start + 2);
    start = body.find(key);
    if (start == std::string::npos)
        return;
    size_t end = body.find("\n", start);
    if (end == std::string::npos)
        end = body.length();
    fullLine = body.substr(start, end - start);

    size_t pos = 0;
    while ((pos = body.find(fullLine, pos)) != std::string::npos) 
        body.erase(pos, fullLine.length() + 1); // +1 = \n
}

int serveUpload(const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{
    (void)requestLocation;
    int storageStatus = isStorageAllowed(serverOne);
    std::string uploadPath = "upload/" + getUploadFilename(http_request); // file.txt if no name
    if (storageStatus == 0 && (http_request.getPath() == "/upload" || http_request.getPath() == "/upload/"))
    {
        //std::cout << BLUE << "uploadPath es " << uploadPath << RESET << std::endl;
        int fd = open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1)
        {
            std::cerr << RED << "open(): " << strerror(errno) <<  RESET << std::endl;
            http_response.setError(getErrorPath(serverOne, 500), 500, "Upload Unavailable");
            http_response.respondInClient(client_fd);
            return (1);
        }
        else
        {
            std::string body = http_request.getBody();
            trimWebKitFormBoundary(body);
            if (write(fd, body.c_str(), body.size()) == -1)
            {
                close(fd);
                http_response.setError(getErrorPath(serverOne, 500), 500, "Upload Unavailable");
                http_response.respondInClient(client_fd);
                return (1);
            }
            close(fd);
        }

        std::string body = getFormSuccessBody();

        std::string response =
            "HTTP/1.1 201 Created\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + UtilsCC::to_stringCC(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;

        send(client_fd, response.c_str(), response.size(), 0);
        http_response.respondInClient(client_fd);
        return (0);
    }
    http_response.setError(getErrorPath(serverOne, storageStatus), storageStatus, "Upload Unavailable");
    http_response.respondInClient(client_fd);
    return (1);
}

int servePost(const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{
    if (!requestLocation || !isMethodAllowed(requestLocation->getMethods(), "POST"))
    {
        http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
        http_response.respondInClient(client_fd);
        return (1);
    }
    if (http_request.exceedsMaxBodySize(serverOne.getClientMaxBodySize()))
    {
        http_response.setError(getErrorPath(serverOne, 413), 413, "Payload Too Large");
        http_response.respondInClient(client_fd);
        return (1);
    }
    if (isUpload(http_request))
        return (serveUpload(requestLocation, client_fd, http_request, http_response, serverOne));
    else
    {
        std::string body = getFormSuccessBody();

        std::string response =
            "HTTP/1.1 201 Created\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + UtilsCC::to_stringCC(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;

        send(client_fd, response.c_str(), response.size(), 0);
        http_response.respondInClient(client_fd);
        return (0);
    }
}

int serveDelete(const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{
    if (!requestLocation || !isMethodAllowed(requestLocation->getMethods(), "DELETE"))
    {
        http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
        http_response.respondInClient(client_fd);
        return (1);
    }
    if (http_request.getPath().find("../") != std::string::npos)
    {
        http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
        http_response.respondInClient(client_fd);
        return (1);
    }
    std::string path = http_request.getPath();
    utils::validatePathWithIndex(path, requestLocation, serverOne);
    std::ifstream file(path.c_str());
    if (!file.good())
    {
        http_response.setError(getErrorPath(serverOne, 404), 404, "Not Found");
        http_response.respondInClient(client_fd);
        return (1);
    }
    if (!utils::hasWXPermission(path) || utils::isDirectory(path))
    {
        http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
        http_response.respondInClient(client_fd);
        return (1);
    }
    if (std::remove(path.c_str()) == -1)
    {
        http_response.setError(getErrorPath(serverOne, 500), 500, "Internal Server Error");
        http_response.respondInClient(client_fd);
        return (1);
    }
    http_response.setResponse(200, "OK");
    http_response.respondInClient(client_fd);
    return (0);
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

void readFromSocket(t_fd_data *fd_data, t_socket *client_socket, int epoll_fd, std::map<int, t_fd_data *> &map_fds)
{
    char buf[4096];
    ssize_t bytesRead = recv(client_socket->socket_fd, buf, sizeof(buf), 0);

    if (bytesRead <= 0)
    {
        //NOTA: SE PODRIA AGRUPAR EL CONTENIDO DEL IF EN UNA FUNCION erase_fd_data()
        int socket_fd = client_socket->socket_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
        close(socket_fd);
        delete(client_socket);
        delete(fd_data);
        map_fds.erase(socket_fd);
        return;
    }
    // leido -> append
    client_socket->readBuffer.append(buf, bytesRead);
}

bool isDirectory(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return (false); // path invalido
    return (S_ISDIR(st.st_mode)); // true si es directorio, false si no
}

std::string makeRelative(std::string path)
{
    if (!path.empty() && path[0] == '/')
        path = path.substr(1);
    return (path);
}

std::string generateAutoindex(const std::string& dirPath)
{
    DIR* dir = opendir(makeRelative(dirPath).c_str());
    if (!dir)
        throw std::runtime_error("Cannot open directory");

    std::ostringstream html;
    html << "<html><head><title>Autoindex</title>"
            "<link rel=\"stylesheet\" href=\"/styles.css\" />"
            "</head>"
         << "<body class=\"autoindex\">"
         << "<h1>Autoindex</h1><ul>";


    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;

        std::string fullPath = dirPath + "/" + name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
            name += "/"; // marcar directorios

        // link vacio, el hiperlink construye la url en el navegador
        html << "<li><a href=\"";
        html << name << "\">" << name << "</a></li>";
    }

    html << "</ul></body></html>";
    closedir(dir);
    return (html.str());
}

std::string getRedirectMessage(int code)
{
    switch(code)
    {
        case 301: return "301 Moved Permanently"; // algunos clientes cambian a GET
        case 302: return "302 Found"; // algunos clientes cambian a GET
        case 307: return "307 Temporary Redirect";
        case 308: return "308 Permanent Redirect";
        default:  return "Redirect";
    }
}

std::string getDirectoryName(const std::string &path)
{
    if (path.empty())
        return ".";
    size_t pos = path.rfind('/');

    if (pos == std::string::npos)
        return ".";
    else if (pos == 0)
        return "/";
    else
        return (path.substr(0, pos));
}

bool hasWXPermission(const std::string &path)
{
    std::string dir = getDirectoryName(path);

    if (access(dir.c_str(), W_OK) == 0 && access(dir.c_str(), X_OK) == 0)
        return (true);
    else
        return (false);
}

// Debe incluir gestion de CGI
int respond(int client_fd, const HttpRequest &http_request, ServerConfig &serverOne)
{
    HttpResponse    http_response;
    const std::string &method = http_request.getMethod();
    const LocationConfig *requestLocation = locationMatchforRequest(http_request.getPath(), serverOne.getLocations());

    if (serveRedirect(requestLocation, client_fd, http_response) == 1)
        return (0);
    if (method == "GET")
        return (serveGet(requestLocation, client_fd, http_request, http_response, serverOne));
    else if (method == "POST")
        return (servePost(requestLocation, client_fd, http_request, http_response, serverOne));
    else if (method == "DELETE")
        return (serveDelete(requestLocation, client_fd, http_request, http_response, serverOne));
    else
    {
        std::cout << "Other method" << std::endl;
        http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
        http_response.respondInClient(client_fd);
        return (1);
    }
}

void handleClientSocket(t_fd_data *fd_data, int epoll_fd, std::map<int, t_fd_data *> &map_fds, epoll_event (&events)[MAX_EVENTS], int i)
{
    t_socket *client_socket = static_cast<t_socket *>(fd_data->data);
    client_socket->server.print();

    if (events[i].events & (EPOLLHUP | EPOLLERR))
    {
        //NOTA: SE PODRIA AGRUPAR EL CONTENIDO DEL IF EN UNA FUNCION erase_fd_data()
        int socket_fd = client_socket->socket_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
        close(socket_fd);
        delete(client_socket);
        delete(fd_data);
        map_fds.erase(socket_fd);
        return ;
    }
    utils::readFromSocket(fd_data, client_socket, epoll_fd, map_fds);
    if (utils::isCompleteRequest(client_socket->readBuffer))
    {
        HttpRequest http_request(client_socket->readBuffer);
        //CHECK CGI Y LLAMARLO SI HACE FALTA
        http_request.printRequest();

        if (utils::respond(client_socket->socket_fd, http_request, client_socket->server) == -1)
        {
            //NOTA: SE PODRIA AGRUPAR EL CONTENIDO DEL IF EN UNA FUNCION erase_fd_data()
            int socket_fd = client_socket->socket_fd;
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
            close(socket_fd);
            delete(client_socket);
            delete(fd_data);
            map_fds.erase(socket_fd);
        }
        else
            client_socket->readBuffer.clear();
    }
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
    server.setDefaultFile("index.html");

    // Location "/"
    LocationConfig loc_root;
    loc_root.setPath("/");
    std::vector<std::string> root_methods;
    root_methods.push_back("GET");
    loc_root.setMethods(root_methods);
    loc_root.setAutoIndex(true);

    // Location "/images/"
    LocationConfig loc_images;
    loc_images.setPath("/images/");
    std::vector<std::string> images_methods;
    images_methods.push_back("GET");
    loc_images.setMethods(images_methods);
    loc_images.setAutoIndex(true);

    // Location "/old_location/"
    LocationConfig loc_old;
    loc_old.setPath("/old_location/");
    std::vector<std::string> old_methods;
    old_methods.push_back("GET");
    old_methods.push_back("POST");
    loc_old.setMethods(old_methods);
    loc_old.setAutoIndex(true);
    loc_old.setRedirect(std::pair<int, std::string>(307, "/new_location/"));

    // Location "/new_location/"
    LocationConfig loc_new;
    loc_new.setPath("/new_location/");
    loc_new.setRootOverride("/new_location");
    std::vector<std::string> new_methods;
    new_methods.push_back("GET");
    new_methods.push_back("POST");
    loc_new.setMethods(new_methods);
    loc_new.setAutoIndex(true);

    // Location "/upload/"
    LocationConfig loc_upload;
    loc_upload.setPath("/upload/");
    std::vector<std::string> upload_methods;
    upload_methods.push_back("POST");
    loc_upload.setMethods(upload_methods);
    loc_upload.setAutoIndex(true);

    // Location "/form_result/"
    LocationConfig loc_form;
    loc_form.setPath("/form_result/");
    loc_form.setRootOverride("/form_result");
    std::vector<std::string> form_methods;
    form_methods.push_back("POST");
    form_methods.push_back("GET");
    loc_form.setMethods(form_methods);
    loc_form.setAutoIndex(true);

    // Add locations to server object
    std::vector<LocationConfig> locations;
    locations.push_back(loc_root);
    locations.push_back(loc_images);
    locations.push_back(loc_upload);
    locations.push_back(loc_form);
    locations.push_back(loc_old);
    locations.push_back(loc_new);
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
