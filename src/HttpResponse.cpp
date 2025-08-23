#include "../include/HttpResponse.hpp"

HttpResponse::HttpResponse()
    : version_(), status_code_(0), status_message_(), headers_(), body_() {}

HttpResponse::HttpResponse(const HttpResponse& other)
    : version_(other.version_), status_code_(other.status_code_),
      status_message_(other.status_message_), headers_(other.headers_),
      body_(other.body_) {}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
    if (this != &other) {
        version_ = other.version_;
        status_code_ = other.status_code_;
        status_message_ = other.status_message_;
        headers_ = other.headers_;
        body_ = other.body_;
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
