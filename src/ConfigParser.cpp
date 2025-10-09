#include "../include/ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>

// Constructor y destructor
ConfigParser::ConfigParser() : line_number_(0) {}

ConfigParser::ConfigParser(const std::string& config_path) 
    : config_file_path_(config_path), line_number_(0) {}

ConfigParser::~ConfigParser() {}

// Limpiar línea de comentarios y espacios
std::string ConfigParser::cleanLine(const std::string& line) {
    std::string cleaned = line;
    
    // Eliminar comentarios
    size_t comment_pos = cleaned.find('#');
    if (comment_pos != std::string::npos) {
        cleaned = cleaned.substr(0, comment_pos);
    }
    
    // Trim espacios
    cleaned.erase(0, cleaned.find_first_not_of(" \t\r\n"));
    cleaned.erase(cleaned.find_last_not_of(" \t\r\n") + 1);
    
    return cleaned;
}

bool ConfigParser::isCommentOrEmpty(const std::string& line) {
    std::string cleaned = cleanLine(line);
    return cleaned.empty() || cleaned[0] == '#';
}

std::vector<std::string> ConfigParser::splitTokens(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (ss >> token) {
        // Eliminar ; al final de directivas
        if (!token.empty() && token[token.size() - 1] == ';') {
            token = token.substr(0, token.size() - 1);
        }
        tokens.push_back(token);
    }
    
    return tokens;
}

void ConfigParser::parse(const std::string& config_path) {
    config_file_path_ = config_path;
    parse();
}

void ConfigParser::parse() {
    std::ifstream file(config_file_path_.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + config_file_path_);
    }
    
    servers_.clear();
    line_number_ = 0;
    std::string line;
    int open_blocks = 0;
    
    while (std::getline(file, line)) {
        line_number_++;
        current_line_ = line;
        
        if (isCommentOrEmpty(line)) {
            continue;
        }
        
        std::string cleaned_line = cleanLine(line);
        std::vector<std::string> tokens = splitTokens(cleaned_line);
        
        if (tokens.empty()) continue;
        
        if (tokens[0] == "server") {
            if (tokens.size() > 1 && tokens[1] != "{") {
                throw std::runtime_error("Syntax error at line " + 
                    std::to_string(line_number_) + ": expected '{' after 'server'");
            }
            open_blocks++;
            parseServerBlock(file);
            open_blocks--;
        } else if (tokens[0] == "}") {
            if (open_blocks == 0) {
                throw std::runtime_error("Unexpected '}' at line " + std::to_string(line_number_));
            }
            open_blocks--;
        } else {
            throw std::runtime_error("Unexpected directive at line " + 
                std::to_string(line_number_) + ": " + tokens[0]);
        }
    }
    
    if (open_blocks > 0) {
        throw std::runtime_error("Unclosed block detected in configuration file");
    }
    
    validateConfig();
}

bool ConfigParser::isValidMethod(const std::string& method) const {
    return method == "GET" || method == "POST" || method == "DELETE";
}


bool ConfigParser::isValidPort(int port) const {
    return port >= 1 && port <= 65535;
}

bool ConfigParser::isValidHost(const std::string& host) const {
    if (host == "*" || host == "0.0.0.0") {
        return true; // Wildcard o todas las interfaces
    }

    struct sockaddr_in sa;
    return inet_pton(AF_INET, host.c_str(), &(sa.sin_addr)) != 0; // Validar dirección IPv4
}

void ConfigParser::parseDirective(const std::vector<std::string>& tokens, 
                                 ServerConfig& server, 
                                 LocationConfig* location) {
    if (tokens[0] == "error_page") {
        if (tokens.size() < 3) {
            throw std::runtime_error("error_page requires a code and a path at line " + std::to_string(line_number_));
        }
        int code = std::atoi(tokens[1].c_str());
        std::string path = tokens[2];
        server.addErrorPage(code, path);
    }
    else if (tokens[0] == "listen") {
        if (tokens.size() < 2) {
            throw std::runtime_error("listen requires a host:port or port at line " + 
                                     std::to_string(line_number_));
        }

        std::string host = "*";
        int port = -1;

        size_t colon_pos = tokens[1].find(':');
        if (colon_pos != std::string::npos) {
            host = tokens[1].substr(0, colon_pos);
            port = std::atoi(tokens[1].substr(colon_pos + 1).c_str());
        } else {
            port = std::atoi(tokens[1].c_str());
        }

        if (!isValidPort(port)) {
            throw std::runtime_error("Invalid port number at line " + 
                                     std::to_string(line_number_) + ": " + tokens[1]);
        }

        if (!isValidHost(host)) {
            throw std::runtime_error("Invalid host at line " + 
                                     std::to_string(line_number_) + ": " + host);
        }

        server.setHost(host);
        server.setPort(port);
    }
    else if (tokens[0] == "root") {
        if (tokens.size() < 2) throw std::runtime_error("root requires a path");
        struct stat info;
        if (stat(tokens[1].c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
            throw std::runtime_error("Invalid root directory at line " + 
                                     std::to_string(line_number_) + ": " + tokens[1]);
        }
        if (location) {
            location->setRootOverride(tokens[1]);
        } else {
            server.setDocumentRoot(tokens[1]);
        }
    }
    else if (tokens[0] == "allowed_methods") {
        if (tokens.size() < 2) throw std::runtime_error("allowed_methods requires methods");
        std::vector<std::string> methods(tokens.begin() + 1, tokens.end());
        for (size_t i = 0; i < methods.size(); ++i) {
            if (!isValidMethod(methods[i])) {
                throw std::runtime_error("Invalid HTTP method at line " + 
                                         std::to_string(line_number_) + ": " + methods[i]);
            }
        }
        if (location) {
            location->setMethods(methods);
        }
    }
    else if (tokens[0] == "autoindex") {
        if (tokens.size() < 2) throw std::runtime_error("autoindex requires on/off");
        if (tokens[1] != "on" && tokens[1] != "off") {
            throw std::runtime_error("Invalid value for autoindex at line " + 
                                     std::to_string(line_number_) + ": " + tokens[1]);
        }
        bool autoindex = (tokens[1] == "on");
        if (location) {
            location->setAutoIndex(autoindex);
        }
    }
    else {
        throw std::runtime_error("Unknown directive at line " + 
                                 std::to_string(line_number_) + ": " + tokens[0]);
    }
}

void ConfigParser::parseServerBlock(std::ifstream& file) {
    ServerConfig server;
    std::string line;
    
    while (std::getline(file, line)) {
        line_number_++;
        current_line_ = line;
        
        if (isCommentOrEmpty(line)) {
            continue;
        }
        
        std::string cleaned_line = cleanLine(line);
        if (cleaned_line == "}") {
            break; // Fin del bloque server
        }
        
        std::vector<std::string> tokens = splitTokens(cleaned_line);
        if (tokens.empty()) continue;
        
        if (tokens[0] == "location") {
            if (tokens.size() < 2) {
                throw std::runtime_error("Syntax error at line " + 
                    std::to_string(line_number_) + ": location requires a path");
            }
            parseLocationBlock(file, server);
        } else {
            parseDirective(tokens, server);
        }
    }
    
    std::map<int, std::string> errpages = server.getErrorPages();
    if (errpages.find(404) == errpages.end()) {
        server.addDefaultErrorPage(404, "/var/www/html/404NotFound.html");
    }
    servers_.push_back(server);
}

void ConfigParser::parseLocationBlock(std::ifstream& file, ServerConfig& server) {
    std::string line;
    std::string location_path = splitTokens(cleanLine(current_line_))[1];
    
    LocationConfig location;
    location.setPath(location_path);
    
    while (std::getline(file, line)) {
        line_number_++;
        current_line_ = line;
        
        if (isCommentOrEmpty(line)) {
            continue;
        }
        
        std::string cleaned_line = cleanLine(line);
        if (cleaned_line == "}") {
            break; // Fin del bloque location
        }
        
        std::vector<std::string> tokens = splitTokens(cleaned_line);
        if (tokens.empty()) continue;
        
        parseDirective(tokens, server, &location);
    }
    
    server.setLocations(server.getLocations());
    // Necesitas agregar el location al server
    std::vector<LocationConfig> locations = server.getLocations();
    locations.push_back(location);
    server.setLocations(locations);
}

// Métodos de validación
void ConfigParser::validateConfig() const {
    std::set<std::pair<std::string, int>> used_listens;
    for (size_t i = 0; i < servers_.size(); ++i) {
        const ServerConfig& server = servers_[i];
        
        if (isValidPort(server.getPort()) == false) { // Validar que el servidor tenga al menos un puerto configurado
            throw std::runtime_error("Invalid port number: " + std::to_string(server.getPort()));
        }
        
        if (server.getDocumentRoot().empty()) { // Validar que el servidor tenga un directorio raíz configurado
            throw std::runtime_error("Root directory not specified for server");
        }

        std::pair<std::string, int> listen_key = {server.getHost(), server.getPort()};
        if (used_listens.find(listen_key) != used_listens.end()) { // Verificar conflictos en los puertos
            throw std::runtime_error("Port conflict detected: " + server.getHost() + ":" + 
                                     std::to_string(server.getPort()));
        }
        used_listens.insert(listen_key);
    }
}

const std::vector<ServerConfig>& ConfigParser::getServers() const {
    return servers_;
}

void ConfigParser::printConfig() const {
    for (size_t i = 0; i < servers_.size(); ++i) {
        const ServerConfig& server = servers_[i];
        std::cout << "Server " << i + 1 << ":\n";
        std::cout << "  Port: " << server.getPort() << "\n";
        std::cout << "  Root: " << server.getDocumentRoot() << "\n";
        
        const std::vector<LocationConfig>& locations = server.getLocations();
        for (size_t j = 0; j < locations.size(); ++j) {
            const LocationConfig& loc = locations[j];
            std::cout << "  Location: " << loc.getPath() << "\n";
            std::cout << "    Methods: ";
            std::vector<std::string> methods = loc.getMethods();
            for (size_t k = 0; k < methods.size(); ++k) {
                std::cout << methods[k] << " ";
            }
            std::cout << "\n";
        }
    }
}