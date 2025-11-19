#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <map>
#include <cstdlib>
#include <algorithm>

#include "macros.hpp"


class HttpRequest {
    private:
        std::string method_;
        std::string path_;
        std::string version_;
        std::map<std::string, std::string> headers_;
        std::string body_;

    public:
        HttpRequest();
        HttpRequest(const std::string& str);
        HttpRequest(const HttpRequest& other);
        HttpRequest& operator=(const HttpRequest& other);
        ~HttpRequest();

        const std::string& getMethod() const;
        const std::string& getPath() const;
        const std::string& getVersion() const;
        const std::map<std::string, std::string>& getHeaders() const;
        const std::string& getBody() const;

        void setMethod(const std::string& method);
        void setPath(const std::string& path);
        void setVersion(const std::string& version);
        void setHeaders(const std::map<std::string, std::string>& headers);
        void setBody(const std::string& body);

        void printRequest() const;
        void trimPathSlashes(std::string &path);
        void parseRequest(const std::string &str);
        bool exceedsMaxBodySize(size_t client_maxbodysize) const;
        std::string findHeader(const std::map<std::string, std::string>& headers, const std::string& key) const;
};

#endif
