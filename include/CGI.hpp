#pragma once

#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>

#include "../include/initServer.hpp"
#include "../include/utils.hpp"
#include "../include/ServerConfig.hpp"
#include "utilsCC.hpp"
#include <string>


namespace CGI
{

	class Handler
	{
		private:
			std::string cgi_;
			std::string nameScript_;
			std::string pathScript_;
			char **env_;
			std::string request_;
			t_server_context *server_context_;
			t_client_socket *client_socket_;
			bool chunked_;
			std::string request_body_;
		public:
			// Canonical
			Handler();
			Handler(const Handler& other);
			Handler& operator=(const Handler& other);
			~Handler();

			// Getters
			std::string getCgi() const;
			std::string getNameScript() const;
			std::string getPathScript() const;
			char** getEnv() const;
			std::string& getRequest() const;
			t_server_context* getServerContext() const;
			t_client_socket* getClientSocket() const;
			bool isChunked() const;
			std::string getRequestBody() const;

			// Setters
			void setCgi(const std::string &cgi);
			void setNameScript(const std::string &nameScript);
			void setPathScript(const std::string &pathScript);
			void setEnv(char** env);
			void setRequest(const std::string &request);
			void setServerContext(t_server_context *server_context);
			void setClientSocket(t_client_socket *client_socket);
			void setChunked(const std::map<std::string, std::string> &headers);
			void setRequestBody(const std::string &requestBody);

			// Methods
			void printAttributes() const;
			int startCGI();
	};
}
