#include "../include/HttpRequest.hpp"

HttpRequest::HttpRequest() : method_(), path_(), version_(), headers_(), body_() {}
HttpRequest::HttpRequest(const std::string &str) : method_(), path_(), version_(), headers_(), body_() { parseRequest(str); }
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


void HttpRequest::parseRequest(const std::string &str)
{
    std::istringstream stream(str);
    std::string line;

    // Request line
    if (!std::getline(stream, line))
        return;
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1, 1);

    std::istringstream requestLine(line);
    requestLine >> method_ >> path_ >> version_;

    // Headers
    headers_.clear();
    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1, 1);

        if (line.empty())
            break;

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);

            // Quitar espacios al inicio del value
            size_t i = 0;
            while (i < value.size() && (value[i] == ' ' || value[i] == '\t'))
                ++i;
            value = value.substr(i);

            headers_[key] = value;
        }
    }

    // Body
    std::ostringstream bodyStream;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1, 1);
        bodyStream << line << "\n";
    }

    body_ = bodyStream.str();
    if (!body_.empty() && body_[body_.size() - 1] == '\n')
        body_.erase(body_.size() - 1, 1);
}

bool HttpRequest::exceedsMaxBodySize(size_t client_maxbodysize) const
{
    if (this->getBody().size() > client_maxbodysize)
        return (true);
    else
        return (false);
}
