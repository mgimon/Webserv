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
int isLocation(ServerConfig &serverOne, const std::string &path)
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
            std::string root;
            if (!requestLocation->getRootOverride().empty())
                root = requestLocation->getRootOverride();
            else
                root = serverOne.getDocumentRoot();
            indexToServe = getFirstValidFile(requestLocation->getLocationIndexFiles(), root);
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

}

std::string getErrorPath(ServerConfig &serverOne, int errcode)
{
    std::string errorpath = serverOne.getDocumentRoot() + "/" + serverOne.getErrorPageName(errcode);
    if (!errorpath.empty() && errorpath[0] == '/')
        errorpath = errorpath.substr(1);
    
    return (errorpath);
}

const LocationConfig *getRedirectLocationMatch(const HttpRequest &http_request, ServerConfig &serverOne, const LocationConfig *requestLocation)
{
    std::vector<LocationConfig> Locations = serverOne.getLocations();
    
    for (std::vector<LocationConfig>::iterator it = Locations.begin(); it < Locations.end(); ++it)
    {
        if(http_request.getPath() == it->getPath() + "/")
            return &(*it);
    }
    return (requestLocation);
}

int serveRedirect(const HttpRequest &http_request, ServerConfig &serverOne, const LocationConfig *requestLocation, int client_fd, HttpResponse &http_response)
{
    const LocationConfig *redirectLocationMatch = getRedirectLocationMatch(http_request, serverOne, requestLocation);
    std::pair<int, std::string> redirect = redirectLocationMatch->getRedirect();
    if (redirect.first != 0)
    {
        if (redirect.second.substr(0, 2) == "./")
            http_response.setRedirectResponse(redirect.first, redirect.second.substr(1)); // quita el .
        else
            http_response.setRedirectResponse(redirect.first, redirect.second);
        std::cout << YELLOW << "Serving redirect!" << RESET << std::endl;
        http_response.respondInClient(client_fd);
    }
    return (-1);
}

//respondGet expects a path as ./(...)
//if keep_alive is -1 when returned, respond will flush the connection
//keep_alive/close is set in response too, but this is not essential for http 1.0
//respondeGet already handles update of Connection:close headers
int serveGet(const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{
    int keep_alive = checkConnectionClose(http_request, http_response);

    if (!requestLocation)
    {
        http_response.setError(getErrorPath(serverOne, 404), 404, "Not Found");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    std::string path = http_request.getPath();

    // asking for raw root (serves first valid index from vector)
    if (isLocation(serverOne, path) == 0)
    {
        validatePathWithIndex(path, requestLocation, serverOne);
        if (isMethodAllowed(requestLocation->getMethods(), "GET"))
            return respondGet(serverOne, client_fd, path, http_request, http_response);
        else
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
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
                if (http_response.respondInClient(client_fd) == -1)
                    return (-1);
                return (keep_alive);
            }
            else
            {
                std::string autoindex_page = utils::generateAutoindexRoot(serverOne.getDocumentRoot(), path);
                http_response.setResponse(200, autoindex_page);
                if (http_response.respondInClient(client_fd) == -1)
                    return (-1);
                return (keep_alive);
            }
        }
        if (isMethodAllowed(requestLocation->getMethods(), "GET"))
            return respondGet(serverOne, client_fd, serverOne.getDocumentRoot() + path, http_request, http_response);
        else
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }
    }
    // inside location
    else if (isLocation(serverOne, path) == 1)
    {
        std::string tempPath = path;
        trimPathSlash(tempPath);
        if (!isMethodAllowed(requestLocation->getMethods(), "GET"))
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }
        if (isDirectory(tempPath) && !isRawLocationRequest(serverOne, path))
        {
            if (requestLocation->getAutoIndex() == false)
            {
                http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
                if (http_response.respondInClient(client_fd) == -1)
                    return (-1);
                return (keep_alive);
            }
            else
            {
                std::string autoindex_page = generateAutoindexLocation(path + "/");
                http_response.setResponse(200, autoindex_page);
                if (http_response.respondInClient(client_fd) == -1)
                    return (-1);
                return (keep_alive);
            }
        }
        if (isRawLocationRequest(serverOne, path) && requestLocation->getAutoIndex() == true)
        {
            std::string autoindex_page = generateAutoindexLocation(path);
            http_response.setResponse(200, autoindex_page);
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }
        if (!requestLocation->getRootOverride().empty() && requestLocation->getLocationIndexFiles().empty())
        {
            http_response.setError(getErrorPath(serverOne, 404), 404, "Not Found");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }
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
                return respondGet(serverOne, client_fd, "." + path, http_request, http_response);
        }
        else
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }
    }
    return (keep_alive);
    
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

std::string getCustomResponse(std::string title, std::string paragraph)
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
    "    <header><h1>" + title + "</h1></header>\n"
    "    <main>\n"
    "        <div class=\"maindiv_first\">\n"
    "            <p>" + paragraph + "</p>\n"
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
    int keep_alive = checkConnectionClose(http_request, http_response);
    int storageStatus = isStorageAllowed(serverOne);
    std::string uploadPath = "upload/" + getUploadFilename(http_request); // file.txt if no name
    if (storageStatus == 0 && (http_request.getPath() == "/upload" || http_request.getPath() == "/upload/"))
    {
        int fd = open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1)
        {
            std::cerr << RED << "open(): " << strerror(errno) <<  RESET << std::endl;
            http_response.setError(getErrorPath(serverOne, 500), 500, "Upload Unavailable");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }
        else
        {
            std::string body = http_request.getBody();
            trimWebKitFormBoundary(body);
            if (write(fd, body.c_str(), body.size()) < 1)
            {
                close(fd);
                http_response.setError(getErrorPath(serverOne, 500), 500, "Upload Unavailable");
                http_response.respondInClient(client_fd);
                return (std::cerr << RED << "Write failed writing in client fd" << RESET << std::endl, -1);
            }
            close(fd);
        }

        std::string body = getFormSuccessBody();

        std::string response =
            "HTTP/1.0 201 Created\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + UtilsCC::to_stringCC(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;

        if (send(client_fd, response.c_str(), response.size(), 0) < 1)
            return (std::cerr << RED << "Send failed writing in client fd" << RESET << std::endl, -1);
        //http_response.respondInClient(client_fd);
        return (-1);
    }
    http_response.setError(getErrorPath(serverOne, storageStatus), storageStatus, "Upload Unavailable");
    if (http_response.respondInClient(client_fd) == -1)
        return (-1);
    return (keep_alive);
}

int servePost(const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{
    int keep_alive = checkConnectionClose(http_request, http_response);

    if (!requestLocation || !isMethodAllowed(requestLocation->getMethods(), "POST"))
    {
        http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    if (http_request.exceedsMaxBodySize(serverOne.getClientMaxBodySize()))
    {
        http_response.setError(getErrorPath(serverOne, 413), 413, "Payload Too Large");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    if (isUpload(http_request))
        return (serveUpload(requestLocation, client_fd, http_request, http_response, serverOne));
    else
    {
        std::string body = getFormSuccessBody();

        std::string response =
            "HTTP/1.0 201 Created\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + UtilsCC::to_stringCC(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;

        if (send(client_fd, response.c_str(), response.size(), 0) < 1)
            return (std::cerr << RED << "Send failed writing in client fd" << RESET << std::endl, -1);
        //http_response.respondInClient(client_fd);
        return (-1);
    }
}

int deleteFile(int keep_alive, const std::string &path, int client_fd, HttpResponse &http_response, ServerConfig &serverOne)
{
    if (std::remove(path.c_str()) == -1)
    {
        http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden"); // 500?
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    http_response.setResponse(200, "OK");
    if (http_response.respondInClient(client_fd) == -1)
        return (-1);
    return (keep_alive);
}

int serveDelete(const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{
    int keep_alive = checkConnectionClose(http_request, http_response);

    if (!requestLocation)
    {
        http_response.setError(getErrorPath(serverOne, 404), 404, "Not Found");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    std::string path = http_request.getPath();

    // asking for raw root (serves first valid index from vector)
    if (isLocation(serverOne, path) == 0)
    {
        http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    // inside of root
    else if (isLocation(serverOne, path) == -1)
    {
        if (isDirectory(serverOne.getDocumentRoot() + path))
        {
            http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }
        if (isMethodAllowed(requestLocation->getMethods(), "DELETE"))
            return deleteFile(keep_alive, serverOne.getDocumentRoot() + path, client_fd, http_response, serverOne); // return respondGet(serverOne, client_fd, serverOne.getDocumentRoot() + path, http_request, http_response);
        else
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }
    }
    // inside location
    else if (isLocation(serverOne, path) == 1)
    {
        std::string tempPath = path;
        trimPathSlash(tempPath);
        if (!isMethodAllowed(requestLocation->getMethods(), "DELETE"))
        {
            http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }
        if (isRawLocationRequest(serverOne, path) || isDirectory(tempPath))
        {
            http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }
        if (!requestLocation->getRootOverride().empty() && requestLocation->getLocationIndexFiles().empty())
        {
            http_response.setError(getErrorPath(serverOne, 404), 404, "Not Found");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
            return (keep_alive);
        }

        std::cout << PINK << "." + path << RESET << std::endl;
        return deleteFile(keep_alive, "." + path, client_fd, http_response, serverOne);
    }
    return (keep_alive);
    
}

int checkConnectionClose(const HttpRequest &http_request, HttpResponse &http_response)
{
    int keep_alive = 1;
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
    return (keep_alive);
}

int respondGet(ServerConfig &serverOne, int client_fd, std::string path, const HttpRequest &http_request, HttpResponse &http_response)
{
    http_response.buildResponse(path, serverOne);
    int keep_alive = checkConnectionClose(http_request, http_response);
    if (http_response.respondInClient(client_fd) == -1)
        return (-1);

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

void readFromSocket(t_client_socket *client_socket, int epoll_fd, std::map<int, t_fd_data> &map_fds)
{
    char buf[4096];
    ssize_t bytesRead = recv(client_socket->socket_fd, buf, sizeof(buf), 0);

    if (bytesRead <= 0)
    {
        if (!client_socket->cgi)
        {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket->socket_fd, NULL);
            close(client_socket->socket_fd);
            map_fds.erase(client_socket->socket_fd);
            delete(client_socket);
        }
        return;
    }
    client_socket->readBuffer.append(buf, bytesRead);
}

bool isDirectory(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return (false);
    return (S_ISDIR(st.st_mode));
}

std::string makeRelative(std::string path)
{
    if (!path.empty() && path[0] == '/')
        path = path.substr(1);
    return (path);
}

std::string generateAutoindexRoot(const std::string& Path, const std::string& directory)
{
    const std::string dirPath = Path + directory;
    DIR* dir = opendir(makeRelative(dirPath).c_str());
    if (!dir)
        throw std::runtime_error("Cannot open directory");

    std::ostringstream html;
    html << "<html><head><title>Autoindex</title>"
         << "<link rel=\"stylesheet\" href=\"/styles.css\" />"
         << "</head><body class=\"autoindex\">"
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

        std::string href = directory;
        if (!href.empty() && href[href.size() - 1] != '/')
            href += "/";
        href += name;

        html << "<li><a href=\"" << href << "\">" << name << "</a></li>";
    }

    html << "</ul></body></html>";
    closedir(dir);
    return html.str();
}

std::string generateAutoindexLocation(const std::string& dirPath)
{
    DIR* dir = opendir(makeRelative(dirPath).c_str());
    if (!dir)
        throw std::runtime_error("Cannot open directory");

    std::ostringstream html;
    html << "<html><head><title>Autoindex</title>"
         << "<link rel=\"stylesheet\" href=\"/styles.css\" />"
         << "</head><body class=\"autoindex\">"
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

        std::string href = dirPath;
        if (!href.empty() && href[href.size() - 1] != '/')
            href += "/";
        href += name;

        html << "<li><a href=\"" << href << "\">" << name << "</a></li>";
    }

    html << "</ul></body></html>";
    closedir(dir);
    return html.str();
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

char toLowerChar(char c)
{
    return static_cast<char>(tolower(static_cast<unsigned char>(c)));
}

std::string trimQueryString(const std::string &s)
{
    std::string::size_type pos = s.find('?');
    if (pos == std::string::npos)
        return (s);
    return (s.substr(0, pos));
}

std::string extractQueryString(const std::string &s)
{
    std::string::size_type pos = s.find('?');
    if (pos == std::string::npos)
        return ("");
    return (s.substr(pos + 1));
}

void freeCStr(char **env)
{
    if (!env)
        return;
    for (int i = 0; env[i] != NULL; ++i)
        free(env[i]);
    delete[] env;
}

char **allocateCgiEnv(const LocationConfig *requestLocation, const HttpRequest &http_request, ServerConfig &serverOne)
{
    (void)requestLocation;
    int headers = static_cast<int>(http_request.getHeaders().size());
    int size = 11 + headers;
    char **env = new char*[size];
    std::ostringstream oss;
    std::string env_var;

    env_var = "REQUEST_METHOD=" + http_request.getMethod();
    env[0] = strdup(env_var.c_str());
    env_var = "QUERY_STRING=" + extractQueryString(http_request.getPath()); 
    env[1] = strdup(env_var.c_str());
    oss << http_request.getBody().size();
    env_var = "CONTENT_LENGTH=" + oss.str(); 
    env[2] = strdup(env_var.c_str());
    env_var = "CONTENT_TYPE=" + http_request.findHeader(http_request.getHeaders(), "Content-Type"); 
    env[3] = strdup(env_var.c_str());
    if (isLocation(serverOne, http_request.getPath()) == 1)
        env_var = "SCRIPT_NAME=" +  trimQueryString(http_request.getPath());
    else
        env_var = "SCRIPT_NAME=" +  trimQueryString(serverOne.getDocumentRoot() + http_request.getPath());
    env[4] = strdup(env_var.c_str());
    env_var = "PATH_INFO=" + std::string(""); 
    env[5] = strdup(env_var.c_str());
    env_var = "SERVER_NAME=" + serverOne.getServerName();
    env[6] = strdup(env_var.c_str());
    oss.str("");
    oss.clear();
    oss << serverOne.getPort();
    env_var = "SERVER_PORT=" + oss.str();
    env[7] = strdup(env_var.c_str());
    env_var = "SERVER_PROTOCOL=HTTP/1.0"; 
    env[8] = strdup(env_var.c_str());
    env_var = "SERVER_SOFTWARE=" + std::string("webserv/1.0"); 
    env[9] = strdup(env_var.c_str());

    int i = 10;
    for (std::map<std::string, std::string>::const_iterator it = http_request.getHeaders().begin(); it != http_request.getHeaders().end(); ++it)
    {
        env_var = "HTTP_" + toUpper(it->first) + "=" + it->second;
        env[i] = strdup(env_var.c_str());
        i++;
    }
    env[10 + headers] = NULL;

    return (env);
}

int checksSetCGI(t_server_context &server_context, t_client_socket *client_socket, const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{
    std::string cleanPath;
    std::pair<int, std::string> redirect;
    (void)server_context;

    if (!requestLocation)
    {
        http_response.setError(getErrorPath(client_socket->server, 404), 404, "Not Found");
        http_response.respondInClient(client_socket->socket_fd);
        return (-1);
    }

    if (!isMethodAllowed(requestLocation->getMethods(), http_request.getMethod()))
    {
        http_response.setError(getErrorPath(client_socket->server, 405), 405, "Method Not Allowed");
        http_response.respondInClient(client_socket->socket_fd);
        return (-1);
    }

    redirect = requestLocation->getRedirect();
    if (redirect.first != 0)
    {
        http_response.setError(getErrorPath(client_socket->server, 403), 403, "Forbidden");
        http_response.respondInClient(client_socket->socket_fd);
        return (-1);
    }

    if (isLocation(serverOne, http_request.getPath()) == 1)
        cleanPath = trimQueryString("." + http_request.getPath());
    else
        cleanPath = trimQueryString (serverOne.getDocumentRoot() + http_request.getPath());
    
    if (http_request.getMethod() == "GET" || http_request.getMethod() == "DELETE")
    {
        if (access(cleanPath.c_str(), F_OK) != 0)
        {
            if (errno == ENOENT)
                http_response.setError(getErrorPath(serverOne, 404), 404, "Not Found");
            else
                http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
            if (http_response.respondInClient(client_fd) == -1)
                return (-1);
        }
    }
    if (http_request.getMethod() == "POST" && http_request.getBody().size() > serverOne.getClientMaxBodySize())
    {
        http_response.setError(getErrorPath(client_socket->server, 413), 413, "Payload Too Large");
        http_response.respondInClient(client_socket->socket_fd);
        return (-1);
    }
    return (1);
}

int setCGI(t_server_context &server_context, t_client_socket *client_socket, const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{

    if (!checksSetCGI(server_context, client_socket, requestLocation, client_fd, http_request, http_response, serverOne))
        return (-1);

    CGI::Handler CgiHandler;

    CgiHandler.setEnv(allocateCgiEnv(requestLocation, http_request, serverOne)); // recordar liberar
    CgiHandler.setCgi(requestLocation->getPythonCGIExecutable());
    CgiHandler.setNameScript(getCgiScriptNameFromPath(http_request.getPath()));
    if (isLocation(serverOne, http_request.getPath()) == 1)
        CgiHandler.setPathScript( "." + getCgiScriptPathFromPath(http_request.getPath()));
    else
        CgiHandler.setPathScript(getCgiScriptPathFromPath(serverOne.getDocumentRoot() + http_request.getPath()));
    CgiHandler.setRequest(http_request.getMethod());
    CgiHandler.setClientSocket(client_socket);
    CgiHandler.setServerContext(&server_context);
    CgiHandler.setRequestBody(http_request.getBody());
    CgiHandler.printAttributes();

    int r = CgiHandler.startCGI();
    freeCStr(CgiHandler.getEnv());
    if (r == 0)
    {
        http_response.setError(getErrorPath(serverOne, 500), 500, "Internal Server Error");
        http_response.respondInClient(client_fd);
        return (-1);
    }
    else if (r == -1)
    {
        http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
        http_response.respondInClient(client_fd);
        return (-1);
    }
    else
        return (0); // mantiene conexion abierta, iteracion posterior manejara el CGI
}

int respond(t_server_context &server_context, t_client_socket *client_socket, int client_fd, HttpRequest &http_request, ServerConfig &serverOne)
{
    HttpResponse    http_response;
    int             keep_alive = checkConnectionClose(http_request, http_response);
    const std::string &method = http_request.getMethod();
    const LocationConfig *requestLocation = locationMatchforRequest(http_request.getPath(), serverOne.getLocations());

    if (!requestLocation)
    {
        http_response.setError(getErrorPath(serverOne, 404), 404, "Not Found");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }

    std::cout << YELLOW << "Request location is " << requestLocation->getPath() << RESET << std::endl;

    // serve REDIRECT
    if (serveRedirect(http_request, serverOne, requestLocation, client_fd, http_response) == 1)
        return (0);
    // set CGI
    if (http_request.getPath().find(".py") != std::string::npos && !isUpload(http_request) && http_request.getMethod() != "DELETE") // CGI, solo GET .py y POST no upload a .py
    {
        if (locationMatchforRequest(http_request.getPath(), serverOne.getLocations())->getPythonCGIExecutable().empty())
        {
            http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
            http_response.respondInClient(client_fd);
            return (-1);
        }
        return (setCGI(server_context, client_socket, requestLocation, client_fd, http_request, http_response, serverOne));
        
    }

    // serve NORMAL REQUEST
    if (method == "GET")
        return (serveGet(requestLocation, client_fd, http_request, http_response, serverOne));
    else if (method == "POST")
    {
        if (isUpload(http_request))
            http_request.setPath(trimQueryString(http_request.getPath()));
        return (servePost(requestLocation, client_fd, http_request, http_response, serverOne));
    }
    else if (method == "DELETE")
    {
        if (http_request.getPath().find(".py") != std::string::npos)
            http_request.setPath(trimQueryString(http_request.getPath()));
        return (serveDelete(requestLocation, client_fd, http_request, http_response, serverOne));
    }
    else
    {
        std::cout << "Other method" << std::endl;
        http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
}

int extractCgiStatus(std::string &sendBuffer)
{
    std::istringstream iss(sendBuffer);
    std::string line;
    std::string result;
    int statusCode = 0;
    bool firstLine = true;

    while (std::getline(iss, line))
    {
        if (line.find("Status:") == 0)
        {
            std::istringstream statusLine(line.substr(7));
            statusLine >> statusCode;
            continue;
        }
        if (!firstLine) result += "\n";
        result += line;
        firstLine = false;
    }

    sendBuffer = result;
    return (statusCode);
}

std::map<std::string, std::string> extractCgiHeaders(std::string &sendBuffer)
{
    std::istringstream iss(sendBuffer);
    std::string line;
    std::map<std::string, std::string> headers;
    std::string result;
    bool firstLine = true;
    bool headersEnded = false;

    while (std::getline(iss, line))
    {
        if (!headersEnded)
        {
            if (line.empty())
            {
                headersEnded = true;
                continue;
            }
            std::size_t pos = line.find(':');
            if (pos != std::string::npos)
            {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                    value.erase(0, 1);
                headers[key] = value;
                continue;
            }
            else
                headersEnded = true;
        }
        if (!firstLine)
            result += "\n";
        result += line;
        firstLine = false;
    }

    sendBuffer = result;
    return (headers);
}

int respondCGI(t_server_context &server_context, t_client_socket *client_socket)
{
    (void)server_context;
    HttpRequest http_request(client_socket->readBuffer);
    HttpResponse http_response;
    std::string buffer = client_socket->sendBuffer;
    int keep_alive = checkConnectionClose(http_request, http_response);
    //const std::string &method = http_request.getMethod();
    const LocationConfig *requestLocation = locationMatchforRequest(http_request.getPath(), client_socket->server.getLocations());

    if (!requestLocation)
    {
        http_response.setError(getErrorPath(client_socket->server, 404), 404, "Not Found");
        if (http_response.respondInClient(client_socket->socket_fd) == -1)
            return (-1);
        return (keep_alive);
    }

    if (buffer.empty())
    {
        http_response.setError(getErrorPath(client_socket->server, 400), 400, "Bad Request");
        if (http_response.respondInClient(client_socket->socket_fd) == -1)
            return (-1);
        return (keep_alive);
    }

    int cgiStatus = extractCgiStatus(buffer); // elimina la linea Status del buffer

    if (cgiStatus != 0 && cgiStatus != 200 && cgiStatus != 201) // habia CGI "Status: "
    {
        http_response.setError(getErrorPath(client_socket->server, cgiStatus), cgiStatus, "Error");
        if (http_response.respondInClient(client_socket->socket_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    else // no habia CGI "Status: "
    {
        std::map<std::string, std::string> headers = extractCgiHeaders(buffer); // quita las headers CGI
        headers["Connection"] = keep_alive;
        http_response.setResponse(200, buffer);
        http_response.setHeaders(headers);
    }

    if (http_response.respondInClient(client_socket->socket_fd) == -1)
        return (-1);
    return (keep_alive);

}

void removeConnection(t_client_socket *client_socket, int epoll_fd, std::map<int, t_fd_data> &map_fds)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket->socket_fd, NULL);
    close(client_socket->socket_fd);
    map_fds.erase(client_socket->socket_fd);
    delete(client_socket);
}

std::string toUpper(const std::string& str)
{
    std::string result = str;
    for (std::string::size_type i = 0; i < result.size(); ++i)
        result[i] = std::toupper(static_cast<unsigned char>(result[i]));
    return (result);
}

bool ends_with_py(const std::string& str)
{
    const char* suffix = ".py";
    size_t suffix_len = 3;

    if (str.size() < suffix_len)
        return false;
    for (size_t i = 0; i < suffix_len; ++i)
    {
        if (str[str.size() - suffix_len + i] != suffix[i])
            return (false);
    }
    return (true);
}

std::string getCgiScriptNameFromPath(const std::string &path)
{
    size_t pyPos = path.find(".py");
    if (pyPos == std::string::npos)
        return "";

    size_t slashPos = path.rfind('/', pyPos);
    if (slashPos == std::string::npos)
        slashPos = 0;
    else
        slashPos++;

    std::string scriptName = path.substr(slashPos, pyPos - slashPos + 3);

    return (scriptName);
}

std::string getCgiScriptPathFromPath(const std::string &path)
{
    size_t pyPos = path.find(".py");
    size_t endPos = (pyPos != std::string::npos) ? pyPos : path.size();

    size_t slashPos = path.rfind('/', endPos); // ultima '/' antes del final del nombre del script
    if (slashPos == std::string::npos)
        return "";

    return path.substr(0, slashPos);
}

std::string unchunkedBody(const std::string &body)
{
    std::string result;
    std::size_t pos = 0;

    while (pos < body.size())
    {
        std::size_t line_end = body.find("\r\n", pos);
        if (line_end == std::string::npos)
            break;

        // Obtener pasar a decimal, 0 es fin
        std::string size_str = body.substr(pos, line_end - pos);
        long chunk_size = strtol(size_str.c_str(), NULL, 16);
        if (chunk_size == 0)
            break;

        pos = line_end + 2;
        if (pos + chunk_size > body.size())
            break;

        result.append(body.substr(pos, chunk_size));
        pos = pos + chunk_size + 2;
    }

    return result;
}

bool isChunked(const HttpRequest &http_request)
{
    std::map<std::string, std::string> headers = http_request.getHeaders();

    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
    {
        if ((it->first.find("Transfer-Encoding") != std::string::npos) && (it->second == "chunked" || it->second == "Chunked"))
            return (true);
    }
    return (false);
}

void handleClientSocket(t_fd_data &fd_data, uint32_t &events, t_server_context &server_context)
{
    t_client_socket *client_socket = static_cast<t_client_socket *>(fd_data.data);
    client_socket->server.print();
    if (events & (EPOLLHUP | EPOLLERR))
    {
        if (client_socket->cgi)
        {
            std::map<pid_t, t_pid_context>::iterator pids_it = server_context.map_pids.find(client_socket->pid);
            UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds, false);
		    server_context.map_pids.erase(pids_it);
        }
        removeConnection(client_socket, server_context.epoll_fd, server_context.map_fds);    
        return ;
    }
    readFromSocket(client_socket, server_context.epoll_fd, server_context.map_fds);
    if (isCompleteRequest(client_socket->readBuffer))
    {
        HttpRequest http_request(client_socket->readBuffer);
        http_request.printRequest();
        if (isChunked(http_request))
        {
            std::string body = unchunkedBody(http_request.getBody());
            http_request.setBody(body);
        }

        if (respond(server_context, client_socket, client_socket->socket_fd, http_request, client_socket->server) == -1) // Client requests Connection:close, or Error
            removeConnection(client_socket, server_context.epoll_fd, server_context.map_fds);
        else
        {
            client_socket->last_activity_time = time(NULL);
            client_socket->readBuffer.clear();
        }
    }
}

std::string normalizePathForMatch(const std::string &path)
{
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
