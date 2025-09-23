#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "LocationConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
//#include "utils.hpp"

typedef struct s_listen
{
	std::string host;
	int port;
	int backlog;
}	t_listen;

class ServerConfig {
	private:
		int buffer_size_;
		std::string document_root_;
		std::vector<LocationConfig> locations_; // Lista de locations
		std::vector<t_listen> listens_;

		std::string host_;
		std::map<int, std::string> error_pages_;
		long client_max_body_size_;
		std::string server_name_;

	public:
		ServerConfig();
		ServerConfig(int buffer_size, const std::string& document_root);
		ServerConfig(const ServerConfig& other);
		ServerConfig& operator=(const ServerConfig& other);
		~ServerConfig();

		int getBufferSize() const;
		std::string getDocumentRoot() const;
		const std::vector<LocationConfig> &getLocations() const;
		std::vector<t_listen> getListens() const;

		void setBufferSize(int buffer_size);
		void setDocumentRoot(const std::string& document_root);
		void setLocations(const std::vector<LocationConfig>& locations);
		void addListen(t_listen listen);


		std::string getHost() const;
		std::map<int, std::string> getErrorPages() const;
		long getClientMaxBodySize() const;
		std::string getServerName() const;
		
		void setHost(const std::string& host);
		void addErrorPage(int code, const std::string& path);
		void setClientMaxBodySize(long size);
		void setServerName(const std::string& name);

		int getPort();
		void setPort(int port);
};

#endif
