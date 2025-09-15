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

#include "LocationConfig.hpp"

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

	public:
		ServerConfig();
		ServerConfig(int buffer_size, const std::string& document_root);
		ServerConfig(const ServerConfig& other);
		ServerConfig& operator=(const ServerConfig& other);
		~ServerConfig();

		int getBufferSize() const;
		std::string getDocumentRoot() const;
		std::vector<LocationConfig> getLocations() const;
		std::vector<t_listen> getListens() const;

		void setBufferSize(int buffer_size);
		void setDocumentRoot(const std::string& document_root);
		void setLocations(const std::vector<LocationConfig>& locations);
		void addListen(t_listen listen);
};

#endif
