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
    //std::cout << YELLOW << "Redirect int: " << requestLocation->getRedirect().first << RESET << std::endl;
    //std::cout << YELLOW << "Redirect path: " << requestLocation->getRedirect().second << RESET << std::endl;
    //std::cout << YELLOW << "Request location" << requestLocation->getPath() << RESET << std::endl;
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
        //std::cout << PINK << "!!!!!!!!!!!!Raw root!!!!!!!!!!!!" << RESET << std::endl;
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
        //std::cout << PINK << "!!!!!!!!!!!!Inside root!!!!!!!!!!!!" << RESET << std::endl;
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
                //std::cout << PINK << "Autoindex generado aqui: " + serverOne.getDocumentRoot() + path << RESET << std::endl;
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
        //std::cout << PINK << "!!!!!!!!!!!!Inside location!!!!!!!!!!!!" << RESET << std::endl;
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
                //std::cout << PINK << "Autoindex generado aqui: " + path << RESET << std::endl;
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
            {
                //std::cout << PINK << "." + path << RESET << std::endl;
                return respondGet(serverOne, client_fd, "." + path, http_request, http_response);
            }
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
        //std::cout << BLUE << "uploadPath es " << uploadPath << RESET << std::endl;
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
            if (write(fd, body.c_str(), body.size()) == -1)
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

        if (send(client_fd, response.c_str(), response.size(), 0) == -1)
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

        if (send(client_fd, response.c_str(), response.size(), 0) == -1)
            return (std::cerr << RED << "Send failed writing in client fd" << RESET << std::endl, -1);
        //http_response.respondInClient(client_fd);
        return (-1);
    }
}

/*
int serveDelete(const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{
    int keep_alive = checkConnectionClose(http_request, http_response);

    if (http_request.getPath().find("../") != std::string::npos)
    {
        http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    std::cout << PINK << "hellooooooo path is " << http_request.getPath() << RESET << std::endl;
    if (!requestLocation || !isMethodAllowed(requestLocation->getMethods(), "DELETE"))
    {
        http_response.setError(getErrorPath(serverOne, 405), 405, "Method Not Allowed");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    std::string path = http_request.getPath();
    //utils::validatePathWithIndex(path, requestLocation, serverOne);
    trimPathSlash(path);
    std::cout << GRAY << "Delete request path is -->" << path << RESET << std::endl;
    std::ifstream file(path.c_str());
    if (!file.good())
    {
        http_response.setError(getErrorPath(serverOne, 404), 404, "Not Found");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    if (!utils::hasWXPermission(path) || utils::isDirectory(path))
    {
        http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    // Deletion
    if (std::remove(path.c_str()) == -1)
    {
        http_response.setError(getErrorPath(serverOne, 500), 500, "Internal Server Error");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    http_response.setResponse(200, "OK");
    if (http_response.respondInClient(client_fd) == -1)
        return (-1);
    return (keep_alive);
}*/

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
        //std::cout << PINK << "!!!!!!!!!!!!Raw root!!!!!!!!!!!!" << RESET << std::endl;
        http_response.setError(getErrorPath(serverOne, 403), 403, "Forbidden");
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
    // inside of root
    else if (isLocation(serverOne, path) == -1)
    {
        //std::cout << PINK << "!!!!!!!!!!!!Inside root!!!!!!!!!!!!" << RESET << std::endl;
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
        //std::cout << PINK << "!!!!!!!!!!!!Inside location!!!!!!!!!!!!" << RESET << std::endl;
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

void readFromSocket(t_fd_data *fd_data, t_client_socket *client_socket, int epoll_fd, std::map<int, t_fd_data *> &map_fds)
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
    env_var = "QUERY_STRING=" + std::string(""); 
    env[1] = strdup(env_var.c_str());
    oss << http_request.getBody().size();
    env_var = "CONTENT_LENGTH=" + oss.str(); 
    env[2] = strdup(env_var.c_str());
    env_var = "CONTENT_TYPE=" + http_request.findHeader(http_request.getHeaders(), "Content-Type"); 
    env[3] = strdup(env_var.c_str());
    env_var = "SCRIPT_NAME=" + http_request.getPath(); // si es root no aparece /var/www/html ?
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

int respondCGI(t_server_context &server_context, t_client_socket *client_socket, const LocationConfig *requestLocation, int client_fd, const HttpRequest &http_request, HttpResponse &http_response, ServerConfig &serverOne)
{
    //if (!access http_request.getPath())
        //-> 403
    //else
        //-> creamos el objeto CGI Handler y llamamos a startCGI

    CGI::Handler CgiHandler;

    CgiHandler.setEnv(allocateCgiEnv(requestLocation, http_request, serverOne)); // recordar liberar
    CgiHandler.setCgi(requestLocation->getPythonCGIExecutable().c_str());
    CgiHandler.setNameScript(getCgiScriptNameFromPath(http_request.getPath()));
    CgiHandler.setPathScript(getCgiScriptPathFromPath(http_request.getPath()));
    CgiHandler.setRequest(http_request.getMethod());
    CgiHandler.setClientSocket(client_socket);
    CgiHandler.setServerContext(&server_context);

    // meter un respond cualquiera y printear con CgiHandler.printAttributes() tras request de CGI;
    CgiHandler.printAttributes();
    http_response.setError(getErrorPath(serverOne, 100), 100, "Custom");
    http_response.respondInClient(client_fd);
    return (-1);
    // CGI::startCGI();
}

int respond(t_server_context &server_context, t_client_socket *client_socket, int client_fd, const HttpRequest &http_request, ServerConfig &serverOne)
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

    // serve REDIRECT or CGI
    if (serveRedirect(http_request, serverOne, requestLocation, client_fd, http_response) == 1)
        return (0);
    if (http_request.getPath().find(".py") != std::string::npos && !locationMatchforRequest(http_request.getPath(), serverOne.getLocations())->getPythonCGIExecutable().empty())
        return (respondCGI(server_context, client_socket, requestLocation, client_fd, http_request, http_response, serverOne));

    // serve NORMAL REQUEST
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
        if (http_response.respondInClient(client_fd) == -1)
            return (-1);
        return (keep_alive);
    }
}

void removeConnection(t_client_socket *client_socket, t_fd_data *fd_data, int epoll_fd, std::map<int, t_fd_data *> &map_fds)
{
    int socket_fd = client_socket->socket_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
    close(socket_fd);
    delete(client_socket);
    delete(fd_data);
    map_fds.erase(socket_fd);
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

void handleClientSocket(t_fd_data *fd_data, t_server_context &server_context, epoll_event (&events)[MAX_EVENTS], int i)
{
    t_client_socket *client_socket = static_cast<t_client_socket *>(fd_data->data);
    client_socket->server.print();

    if (events[i].events & (EPOLLHUP | EPOLLERR))
    {
        removeConnection(client_socket, fd_data, server_context.epoll_fd, server_context.map_fds);
        return ;
    }
    readFromSocket(fd_data, client_socket, server_context.epoll_fd, server_context.map_fds);
    if (isCompleteRequest(client_socket->readBuffer))
    {
        HttpRequest http_request(client_socket->readBuffer);
        http_request.printRequest();
        //CHECK CGI Y LLAMARLO SI HACE FALTA

        /*NOTA: Hay que hacer que no se entre en response hasta que el processo del cgi este acabdo,
        no se como hacerlo exactamente, la idea seria cambiar la config del epoll para que solo notifique 
        eventos de error con el cliente hasta que el cgi haya acabado, y guardar la respuesta del cgi
        en un buffer para asignarlo al body que devolvemos al cliente
        */

        /*
        REQUEST_METHOD      GET                                 Método HTTP usado por el cliente
        QUERY_STRING	    nombre=Juan&edad=30	                Todo lo que viene después del ? en la URL
        CONTENT_LENGTH	    123	                                Longitud del cuerpo (si POST)
        CONTENT_TYPE	    application/x-www-form-urlencoded	Tipo de datos enviados
        SCRIPT_NAME	        /var/www/html/cgi-bin/test.py       Nombre del script CGI
        PATH_INFO           ""
        SERVER_NAME         localhost
        SERVER_PORT         8080
        SERVER_PROTOCOL     HTTP/1.0
        SERVER_SOFTWARE     webserv/1.0
        REMOTE_ADDR	        192.168.1.2	                        IP del cliente
        REMOTE_PORT
        HTTP_*              cabeceras http convertidas         
        */

        //DENTRO DE RESPOND
        //if (http_request.getPath().find(".py") != std::string::npos && locationMatchforRequest(http_request.getPath(), client_socket->server.getLocations())->isCgi() == true)
            //->entramos en respond-cgi
                //if (!access http_request.getPath())
                    //-> 403
                //else
                    //-> creamos el objeto CGI Handler y llamamos a startCGI
        //else
            //->respond normal y QSLQDQ

        if (respond(server_context, client_socket, client_socket->socket_fd, http_request, client_socket->server) == -1) // Client requests Connection:close, or Error
            removeConnection(client_socket, fd_data, server_context.epoll_fd, server_context.map_fds);
        else
            client_socket->readBuffer.clear();
    }
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
