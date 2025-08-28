#include "../include/HttpRequest.hpp"

HttpRequest::HttpRequest() : method_(), path_(), version_(), headers_(), body_() {}
HttpRequest::HttpRequest(int client_fd) : method_(), path_(), version_(), headers_(), body_() { parseRequest(client_fd); }
HttpRequest::HttpRequest(const HttpRequest& other) : method_(other.method_), path_(other.path_), version_(other.version_), headers_(other.headers_), body_(other.body_) {}
HttpRequest& HttpRequest::operator=(const HttpRequest& other) { if (this != &other) { method_ = other.method_; path_ = other.path_; version_ = other.version_; headers_ = other.headers_; body_ = other.body_; } return *this; }
HttpRequest::~HttpRequest() {}

const std::string& HttpRequest::getMethod() const { return method_; }
const std::string& HttpRequest::getPath() const { return path_; }
const std::string& HttpRequest::getVersion() const { return version_; }
const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return headers_; }
const std::string& HttpRequest::getBody() const { return body_; }

void HttpRequest::setMethod(const std::string& method) { method_ = method; }
void HttpRequest::setPath(const std::string& path) { path_ = path; }
void HttpRequest::setVersion(const std::string& version) { version_ = version; }
void HttpRequest::setHeaders(const std::map<std::string, std::string>& headers) { headers_ = headers; }
void HttpRequest::setBody(const std::string& body) { body_ = body; }

void HttpRequest::printRequest() const {

    std::cout << "REQUEST:" << std::endl;
    std::cout << GREEN << this->getMethod() << " " << this->getPath() << " " << this->getVersion() << RESET << std::endl;

        std::map<std::string, std::string>::const_iterator it = this->getHeaders().begin();
        std::cout << "    Headers:" << std::endl;
        while (it != this->getHeaders().end())
        {
            std::cout << GREEN << "    " << it->first << "  " << it->second << RESET << std::endl;
            ++it;
        }
        std::cout << "    Body:" << std::endl;
        std::cout << GREEN << "    " << this->getBody() << RESET << std::endl;
}

std::string readUntilBody(int client_fd, ssize_t &bytes_read)
{
    char buffer[4096];
    std::string saved;

    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0';
        saved += buffer;
        // detener lectura si ya encontramos el final de headers
        if (saved.find("\r\n\r\n") != std::string::npos)
            break;
    }

    return (saved);
}

void HttpRequest::readBody(int client_fd)
{
    body_.clear();
    std::map<std::string, std::string>::const_iterator it = headers_.find("Content-Length");
    // Si hay body
    if (it != headers_.end())
    {
        int content_length = atoi(it->second.c_str());
        if (content_length > 0)
        {
            ssize_t bytes_read;
            std::string saved = readUntilBody(client_fd, bytes_read);
            size_t pos = saved.find("\r\n\r\n");
            std::string body_part = saved.substr(pos + 4);
            body_ = body_part;

            // Resize
            ssize_t remaining = content_length - body_part.size();
            char buffer[4096];
            while (remaining > 0)
            {
                ssize_t to_read = (remaining > 4096) ? 4096 : remaining;
                ssize_t n = read(client_fd, buffer, to_read);
                if (n <= 0) break;
                body_.append(buffer, n);
                remaining -= n;
            }
        }
    }
}

int HttpRequest::parseRequest(int client_fd)
{
    ssize_t bytes_read;
    std::string saved = readUntilBody(client_fd, bytes_read);
    if (saved.empty())
        return (-1);

    std::istringstream rStream(saved);
    std::string line;

    // Request line
    if (!std::getline(rStream, line))
        return (-1);
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

    std::istringstream first_line(line);
    if (!(first_line >> this->method_ >> this->path_ >> this->version_))
        return (-1);

    // Headers
    headers_.clear();
    while (std::getline(rStream, line) && line != "\r") {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            // limpiar espacios al inicio
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                value.erase(0, 1);
            headers_[key] = value;
        }
    }

    readBody(client_fd);

    return 0;
}
