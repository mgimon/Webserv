#include "../include/ServerConfig.hpp"

/** CANONICAL **/
ServerConfig::ServerConfig() : buffer_size_(4096), document_root_("./") {}
ServerConfig::ServerConfig(int buffer_size, const std::string& document_root) : buffer_size_(buffer_size), document_root_(document_root) {}
ServerConfig::ServerConfig(const ServerConfig& other) : buffer_size_(other.buffer_size_), document_root_(other.document_root_), listens_(other.listens_) {}
ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {buffer_size_ = other.buffer_size_; document_root_ = other.document_root_; listens_ = other.listens_;}
    return *this;
}
ServerConfig::~ServerConfig() {}


/** FUNCTIONS **/
int ServerConfig::getBufferSize() const { return buffer_size_; }
std::string ServerConfig::getDocumentRoot() const { return document_root_; }

const std::vector<LocationConfig> &ServerConfig::getLocations() const { return locations_; }
std::vector<t_listen> ServerConfig::getListens() const { return listens_; }


void ServerConfig::setBufferSize(int buffer_size) { buffer_size_ = buffer_size; }
void ServerConfig::setDocumentRoot(const std::string& document_root) { document_root_ = document_root; }
void ServerConfig::setLocations(const std::vector<LocationConfig>& locations) { locations_ = locations; }
void ServerConfig::addListen(t_listen listen) { listens_.push_back(listen); }