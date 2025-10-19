#include "../include/ServerConfig.hpp"
#define _GLIBCXX_USE_CXX11_ABI 0


/** CANONICAL **/
ServerConfig::ServerConfig() : buffer_size_(4096), client_maxbodysize_(10485760), document_root_("./"), default_file_(""), autoindex_(true) { addDefaultErrorPages(); }
ServerConfig::ServerConfig(int buffer_size, const std::string& document_root) : buffer_size_(buffer_size), client_maxbodysize_(10485760), document_root_(document_root), default_file_(""), autoindex_(true) { addDefaultErrorPages(); }
ServerConfig::ServerConfig(const ServerConfig& other) : buffer_size_(other.buffer_size_), client_maxbodysize_(other.client_maxbodysize_), document_root_(other.document_root_), default_file_(other.default_file_), autoindex_(other.autoindex_), locations_(other.locations_), listens_(other.listens_), defaulterrorpages_(other.defaulterrorpages_) {}
ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {buffer_size_ = other.buffer_size_; client_maxbodysize_ = other.client_maxbodysize_; document_root_ = other.document_root_; default_file_ = other.default_file_; autoindex_ = other.autoindex_; listens_ = other.listens_; defaulterrorpages_ = other.defaulterrorpages_; }
    return *this;
}
ServerConfig::~ServerConfig() {}


/** FUNCTIONS **/
int ServerConfig::getBufferSize() const { return buffer_size_; }
std::string ServerConfig::getDocumentRoot() const { return document_root_; }
std::string ServerConfig::getDefaultFile() const { return default_file_; }
bool ServerConfig::getAutoindex() const { return autoindex_; }
const std::vector<LocationConfig> &ServerConfig::getLocations() const { return locations_; }
std::vector<t_listen> ServerConfig::getListens() const { return listens_; }
size_t ServerConfig::getClientMaxBodySize() const { return client_maxbodysize_; }


void ServerConfig::setBufferSize(int buffer_size) { buffer_size_ = buffer_size; }
void ServerConfig::setDocumentRoot(const std::string& document_root) { document_root_ = document_root; }
void ServerConfig::setDefaultFile(const std::string& default_file) { default_file_ = default_file; }
void ServerConfig::setAutoindex(bool autoindex) { autoindex_ = autoindex; }
void ServerConfig::setLocations(const std::vector<LocationConfig>& locations) { locations_ = locations; }
void ServerConfig::setClientMaxBodySize(size_t client_maxbodysize) { client_maxbodysize_ = client_maxbodysize; }
void ServerConfig::addListen(t_listen listen) { listens_.push_back(listen); }

std::string ServerConfig::getErrorPageName(int errcode) const
{
    for (std::map<int, std::string>::const_iterator it = defaulterrorpages_.begin(); it != defaulterrorpages_.end(); ++it)
    {
        if (it->first == errcode)
            return (it->second);
    }
    return ("500Error.html");
}

// overwrites default error pages if the key already exists
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
    addDefaultErrorPage(413, "413PayloadTooLarge.html");
    addDefaultErrorPage(500, "500Error.html");
}

void ServerConfig::print() const
{
    std::cout << BLUE << "ServerConfig:" << RESET << std::endl;
    std::cout << BLUE << "  Buffer Size: " << RESET << buffer_size_ << std::endl;
    std::cout << BLUE << "  Client Max Body Size: " << RESET << client_maxbodysize_ << std::endl;
    std::cout << BLUE << "  Document Root: " << RESET << document_root_ << std::endl;

    std::cout << BLUE << "  Listens:" << RESET << std::endl;
    for (size_t i = 0; i < listens_.size(); ++i)
    {
        std::cout << "    " << BLUE << "Host: " << RESET << listens_[i].host
                  << ", " << BLUE << "Port: " << RESET << listens_[i].port
                  << ", " << BLUE << "Backlog: " << RESET << listens_[i].backlog
                  << std::endl;
    }

    std::cout << BLUE << "  Default Error Pages:" << RESET << std::endl;
    for (std::map<int, std::string>::const_iterator it = defaulterrorpages_.begin();
         it != defaulterrorpages_.end(); ++it)
    {
        std::cout << "    " << BLUE << "Error " << RESET << it->first
                  << " -> " << it->second << std::endl;
    }

    std::cout << BLUE << "  Locations:" << RESET << std::endl;
    for (size_t i = 0; i < locations_.size(); ++i)
    {
        std::cout << "    " << BLUE << "Location " << RESET << i << std::endl;
        locations_[i].printLocation();
    }
}

void ServerConfig::addErrorPage(int code, const std::string& path) {
    defaulterrorpages_[code] = path;
}

void ServerConfig::setHost(const std::string& host) {
    host_ = host;
}

void ServerConfig::setPort(int port) {
    if (listens_.empty()) {
        t_listen listen;
        listen.host = host_;
        listen.port = port;
        listen.backlog = 128;
        listens_.push_back(listen);
    } else {
        listens_[0].port = port;
    }
}

std::map<int, std::string> ServerConfig::getErrorPages() const {
    return defaulterrorpages_;
}

int ServerConfig::getPort() const {
    if (!listens_.empty())
        return listens_[0].port;
    return 0;
}

std::string ServerConfig::getHost() const {
    if (!listens_.empty())
        return listens_[0].host;
    return host_;
}

const std::vector<std::string>& ServerConfig::getServerIndexFiles() const { 
	return index_files_; 
}
void  ServerConfig::setServerIndexFiles(const std::vector<std::string>& index_files) {
	index_files_ = index_files; 
}