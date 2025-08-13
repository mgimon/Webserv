#include "../include/ServerConfig.hpp"

/** CANONICAL **/
ServerConfig::ServerConfig() : host_("0.0.0.0"), port_(8080), backlog_(10), buffer_size_(4096), document_root_("./") {}
ServerConfig::ServerConfig(const std::string& host, int port, int backlog, int buffer_size, const std::string& document_root) : host_(host), port_(port), backlog_(backlog), buffer_size_(buffer_size), document_root_(document_root) {}
ServerConfig::ServerConfig(const ServerConfig& other) : host_(other.host_), port_(other.port_), backlog_(other.backlog_), buffer_size_(other.buffer_size_), document_root_(other.document_root_) {}
ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) { host_ = other.host_; port_ = other.port_; backlog_ = other.backlog_; buffer_size_ = other.buffer_size_; document_root_ = other.document_root_; }
    return *this;
}
ServerConfig::~ServerConfig() {}


/** METHODS **/
std::string ServerConfig::getHost() const { return host_; }
int ServerConfig::getPort() const { return port_; }
int ServerConfig::getBacklog() const { return backlog_; }
int ServerConfig::getBufferSize() const { return buffer_size_; }
std::string ServerConfig::getDocumentRoot() const { return document_root_; }

void ServerConfig::setHost(const std::string& host) { host_ = host; }
void ServerConfig::setPort(int port) { port_ = port; }
void ServerConfig::setBacklog(int backlog) { backlog_ = backlog; }
void ServerConfig::setBufferSize(int buffer_size) { buffer_size_ = buffer_size; }
void ServerConfig::setDocumentRoot(const std::string& document_root) { document_root_ = document_root; }
