#include "../include/ServerConfig.hpp"

/** CANONICAL **/
ServerConfig::ServerConfig() : buffer_size_(4096), document_root_("./") { addDefaultErrorPages(); }
ServerConfig::ServerConfig(int buffer_size, const std::string& document_root) : buffer_size_(buffer_size), document_root_(document_root) { addDefaultErrorPages(); }
ServerConfig::ServerConfig(const ServerConfig& other) : buffer_size_(other.buffer_size_), document_root_(other.document_root_), locations_(other.locations_), listens_(other.listens_), defaulterrorpages_(other.defaulterrorpages_) {}
ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {buffer_size_ = other.buffer_size_; document_root_ = other.document_root_; listens_ = other.listens_; defaulterrorpages_ = other.defaulterrorpages_; }
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

std::string ServerConfig::getBestmatchingErrorPageName(int errcode, const std::string &errpagename) const
{
    for (std::map<int, std::string>::const_iterator it = defaulterrorpages_.begin(); it != defaulterrorpages_.end(); ++it)
    {
        if (it->first == errcode)
            return (it->second);
    }
    return ("500Error.html");
}

// overwrites errpagenames if key already exists
void ServerConfig::addDefaultErrorPage(int errcode, std::string errpagename)
{
    defaulterrorpages_[errcode] = errpagename;
}

void ServerConfig::addDefaultErrorPages()
{
    addDefaultErrorPage(400, "400BadRequest.html");
    addDefaultErrorPage(403, "403Forbidden.html");
    addDefaultErrorPage(404, "404NotFound.html");
    addDefaultErrorPage(405, "405MethodNotAllowed.html");
    addDefaultErrorPage(500, "500Error.html");
}