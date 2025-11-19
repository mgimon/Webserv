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
		size_t	client_maxbodysize_;
		std::string document_root_;
		std::string default_file_;
		bool        autoindex_; // Las locations deben heredarla! o sobreescribirla
		std::vector<LocationConfig> locations_;
		std::vector<t_listen> listens_;
		std::map<int, std::string> defaulterrorpages_; // Seteado en constructor por defecto. Usar addDefaultErrorPage(int, std::string) para sobreescribir una entrada o anadir

		std::string host_;
		std::string server_name_;
		std::vector<std::string> index_files_;

	public:
		ServerConfig();
		ServerConfig(int buffer_size, const std::string& document_root);
		ServerConfig(const ServerConfig& other);
		ServerConfig& operator=(const ServerConfig& other);
		~ServerConfig();

		int getBufferSize() const;
		std::string getDocumentRoot() const;
		std::string getDefaultFile() const;
		bool getAutoindex() const;
		const std::vector<LocationConfig> &getLocations() const;
		std::vector<t_listen> getListens() const;
		std::string getErrorPageName(int errcode) const;
//      size_t getClientMaxBodySize() const;

		void setBufferSize(int buffer_size);
		void setDocumentRoot(const std::string& document_root);
		void setDefaultFile(const std::string& default_file);
		void setAutoindex(bool autoindex);
		void setLocations(const std::vector<LocationConfig>& locations);
		void setClientMaxBodySize(size_t client_maxbodysize);
		void addDefaultErrorPage(int errcode, std::string errpagename);
		void addListen(t_listen listen);


		std::string getHost() const;
		std::map<int, std::string> getErrorPages() const;
		size_t getClientMaxBodySize() const;
		std::string getServerName() const;
		
		void setHost(const std::string& host);
		void addErrorPage(int code, const std::string& path);
		void setClientMaxBodySize(long size);
		void setServerName(const std::string& name);

		int getPort() const;
		void setPort(int port);
		void addDefaultErrorPages();
		void print() const;

		const std::vector<std::string>& getServerIndexFiles() const;
		void setServerIndexFiles(const std::vector<std::string>&index_files);
		void addLocation(const LocationConfig& location);
};

#endif
