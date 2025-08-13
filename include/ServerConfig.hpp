#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>

class ServerConfig {
	private:
		std::string host_;
		int port_;
		int backlog_;
		int buffer_size_;
		std::string document_root_;

	public:
		ServerConfig();
		ServerConfig(const std::string& host, int port, int backlog, int buffer_size, const std::string& document_root);
		ServerConfig(const ServerConfig& other);
		ServerConfig& operator=(const ServerConfig& other);
		~ServerConfig();

		std::string getHost() const;
		int getPort() const;
		int getBacklog() const;
		int getBufferSize() const;
		std::string getDocumentRoot() const;

		void setHost(const std::string& host);
		void setPort(int port);
		void setBacklog(int backlog);
		void setBufferSize(int buffer_size);
		void setDocumentRoot(const std::string& document_root);
};

#endif
