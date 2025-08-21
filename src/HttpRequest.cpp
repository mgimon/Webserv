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

//TODO : construir path dinamicamente
int HttpRequest::parseRequest(int client_fd) {
    char buffer[4096];
    std::string request;
    ssize_t bytes_read;

    // Leer desde el socket
    while ((bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        request += buffer;
        // detener lectura si ya encontramos el final de headers
        if (request.find("\r\n\r\n") != std::string::npos)
            break;
    }

    if (request.empty())
        return -1;

    std::istringstream request_stream(request);
    std::string line;

    // ---- Primera línea (Request-Line) ----
    if (!std::getline(request_stream, line))
        return -1;
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

    std::istringstream first_line(line);
    if (!(first_line >> method_ >> path_ >> version_))
        return -1;

    // ---- Headers ----
    headers_.clear();
    while (std::getline(request_stream, line) && line != "\r") {
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

    // ---- Body (si hay Content-Length) ----
    body_.clear();
    std::map<std::string, std::string>::const_iterator it = headers_.find("Content-Length");
    if (it != headers_.end()) {
        int content_length = atoi(it->second.c_str());
        if (content_length > 0) {
            // ya puede haber datos del body en el buffer leído
            std::string remaining;
            std::getline(request_stream, remaining, '\0');
            if (!remaining.empty() && remaining[0] == '\r')
                remaining.erase(0, 1);
            body_ = remaining;

            // si falta por leer
            while ((int)body_.size() < content_length) {
                bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
                if (bytes_read <= 0) break;
                body_.append(buffer, bytes_read);
            }
        }
    }

    return 0;
}
