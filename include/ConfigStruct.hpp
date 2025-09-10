#ifndef CONFIG_STRUCT
#define CONFIG_STRUCT

struct LocationConfig {
    std::string path;
    std::vector<std::string> allowed_methods;
    std::string root;
    std::string index;
    bool directory_listing;
    std::string upload_path;
    std::string cgi_pass;
};

struct ServerConfig {
    int port;
    std::string host;
    std::string server_name;
    std::string root;
    std::map<int, std::string> error_pages;
    std::vector<LocationConfig> locations;
};

#endif // !CONFIG_STRUCT
