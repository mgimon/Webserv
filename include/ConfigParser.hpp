#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include <arpa/inet.h>

class ConfigParser {
private:
    std::string config_file_path_;
    std::vector<ServerConfig> servers_;
    std::string current_line_;
    size_t line_number_;
    
    // Métodos de utilidad
    std::string cleanLine(const std::string& line);
    bool isCommentOrEmpty(const std::string& line);
    std::vector<std::string> splitTokens(const std::string& line);
    
    // Métodos de parsing
    void parseServerBlock(std::ifstream& file);
    void parseLocationBlock(std::ifstream& file, ServerConfig& server);
    void parseDirective(const std::vector<std::string>& tokens, ServerConfig& server, LocationConfig* location = nullptr);
    
    // Métodos de validación
    void validateConfig() const;
    bool isValidPort(int port) const;
    bool isValidMethod(const std::string& method) const;
    std::vector<std::string> removeDuplicateMethods(const std::vector<std::string> &methods);
    bool isValidPath(const std::string &path, bool shouldBeDir) const;
    bool isValidHost(const std::string& host) const;

public:
    ConfigParser();
    ConfigParser(const std::string& config_path);
    ~ConfigParser();
    
    void parse(const std::string& config_path);
    void parse(); // Usa la ruta por defecto
    
    const std::vector<ServerConfig>& getServers() const;
    void printConfig() const; // Para debugging
};

#endif