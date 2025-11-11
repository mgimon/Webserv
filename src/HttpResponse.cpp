#include "../include/HttpResponse.hpp"

HttpResponse::HttpResponse()
    : version_("HTTP/1.1"), status_code_(0), status_message_(), headers_(), body_(), content_type_() {}

HttpResponse::HttpResponse(const HttpResponse& other)
    : version_(other.version_), status_code_(other.status_code_),
      status_message_(other.status_message_), headers_(other.headers_),
      body_(other.body_), content_type_(other.content_type_) {}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
    if (this != &other) {
        version_ = other.version_;
        status_code_ = other.status_code_;
        status_message_ = other.status_message_;
        headers_ = other.headers_;
        body_ = other.body_;
        content_type_ = other.content_type_;
    }
    return *this;
}

HttpResponse::~HttpResponse() {}

const std::string& HttpResponse::getVersion() const { return version_; }
int HttpResponse::getStatusCode() const { return status_code_; }
const std::string& HttpResponse::getStatusMessage() const { return status_message_; }
const std::map<std::string, std::string>& HttpResponse::getHeaders() const { return headers_; }
const std::string& HttpResponse::getBody() const { return body_; }

void HttpResponse::setVersion(const std::string& version) { version_ = version; }
void HttpResponse::setStatusCode(int code) { status_code_ = code; }
void HttpResponse::setStatusMessage(const std::string& message) { status_message_ = message; }
void HttpResponse::setHeaders(const std::map<std::string, std::string>& headers) { headers_ = headers; }
void HttpResponse::setBody(const std::string& body) { body_ = body; }

void HttpResponse::setContentType(const std::string &path)
{
    if (path.rfind(".html") != std::string::npos)
        content_type_ = "text/html";
    else if (path.rfind(".css") != std::string::npos)
        content_type_ = "text/css";
    else if (path.rfind(".js") != std::string::npos)
        content_type_ = "application/javascript";
    else if (path.rfind(".png") != std::string::npos)
        content_type_ = "image/png";
    else if (path.rfind(".jpg") != std::string::npos || path.rfind(".jpeg") != std::string::npos)
        content_type_ = "image/jpeg";
    else
        content_type_ = "text/plain";
}

void HttpResponse::setResponse(int statusCode, const std::string &body)
{
    std::map<std::string, std::string> responseHeaders;
    std::ostringstream oss;

    this->setBody(body);
    oss << body.size();

    this->setStatusCode(statusCode);
    this->setStatusMessage("OK");
    responseHeaders.insert(std::make_pair("Content-Type", this->content_type_));
    responseHeaders.insert(std::make_pair("Content-Length", oss.str()));
    responseHeaders.insert(std::make_pair("Connection", "close"));
    this->setHeaders(responseHeaders);
}

void HttpResponse::setError(const std::string &filepath, int statusCode, const std::string &error_msg) {

    std::ifstream file(filepath.c_str());

    if (file)
    {
        std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        this->setBody(body);
        std::ostringstream oss;
        oss << body.size();

        this->setStatusCode(statusCode);
        this->setStatusMessage(error_msg);
        std::map<std::string, std::string> responseHeaders;
        responseHeaders.insert(std::make_pair("Content-Type", "text/html"));
        responseHeaders.insert(std::make_pair("Content-Length", oss.str()));
        responseHeaders.insert(std::make_pair("Connection", "close"));
        this->setHeaders(responseHeaders);
    }
    else
    {

        //std::string errorbackup = "<html><body><h1>" + statusCode + error_msg + "</h1></body></html>";
        std::ostringstream oss_code;
        oss_code << statusCode;
        std::string errorbackup = "<html><body><h1>" + oss_code.str() + " " + error_msg + "</h1></body></html>";

        this->setBody(errorbackup);
        std::ostringstream oss;
        oss << errorbackup.size();

        this->setStatusCode(statusCode);
        this->setStatusMessage(error_msg);
        std::map<std::string, std::string> responseHeaders;
        responseHeaders.insert(std::make_pair("Content-Type", "text/html"));
        responseHeaders.insert(std::make_pair("Content-Length", oss.str()));
        responseHeaders.insert(std::make_pair("Connection", "close"));
        this->setHeaders(responseHeaders);
    }

}

void HttpResponse::set200(std::ifstream &file)
{
    std::map<std::string, std::string> responseHeaders;
    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::ostringstream oss;

    this->setBody(body);
    oss << body.size();

    this->setStatusCode(200);
    this->setStatusMessage("OK");
    responseHeaders.insert(std::make_pair("Content-Type", this->content_type_));
    responseHeaders.insert(std::make_pair("Content-Length", oss.str()));
    responseHeaders.insert(std::make_pair("Connection", "close"));
    this->setHeaders(responseHeaders);
}

void HttpResponse::setRedirectResponse(int statusCode, const std::string &path)
{
    std::map<std::string, std::string> responseHeaders;
    std::string body = "";
    std::ostringstream oss;

    this->setBody(body);
    oss << body.size();

    this->setStatusCode(statusCode);
    this->setStatusMessage(utils::getRedirectMessage(statusCode));
    responseHeaders.insert(std::make_pair("Content-Type", this->content_type_));
    responseHeaders.insert(std::make_pair("Content-Length", oss.str()));
    responseHeaders.insert(std::make_pair("Connection", "close"));
    responseHeaders.insert(std::make_pair("Location", path));
    this->setHeaders(responseHeaders);
}

void HttpResponse::forceConnectionClose() {
    std::map<std::string, std::string> headers = this->getHeaders();
    headers["Connection"] = "close";
    this->setHeaders(headers);
}

void HttpResponse::buildResponse(std::string path, ServerConfig &serverOne) {

    std::ifstream file(path.c_str());
    
    if (!file)
    {
        if (errno == EACCES)
            setError(utils::getErrorPath(serverOne, 403), 403, "Forbidden");
        else
            setError(utils::getErrorPath(serverOne, 404), 404, "Not Found");
    }
    else
        set200(file);
}

void HttpResponse::respondInClient(int client_fd)
{
    std::ostringstream response;
    std::map<std::string, std::string> headers = this->getHeaders();

    response << this->getVersion() << " " << this->getStatusCode()
                   << " " << this->getStatusMessage() << "\r\n";
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
        response << it->first << ": " << it->second << "\r\n";
    response << "\r\n" << this->getBody();

    write(client_fd, response.str().c_str(), response.str().size());
}
