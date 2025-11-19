#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../include/initServer.hpp"
#include "../include/ServerConfig.hpp"
#include <string>



namespace CGI
{
	int startCGI(char *cgi, char *nameScript, char *pathScript, char **env, std::string request, t_server_context &server_context, t_client_socket *client_socket);

	class Handler
	{
		private:
			const char *cgi_;
			std::string nameScript_;
			std::string pathScript_;
			char **env_;
			std::string request_;
			t_server_context *server_context_;
			t_client_socket *client_socket_;
		public:
			// Canonical
			Handler();
			Handler(const Handler& other);
			Handler& operator=(const Handler& other);
			~Handler();

			// Getters
			const char* getCgi() const;
			std::string getNameScript() const;
			std::string getPathScript() const;
			char** getEnv() const;
			std::string& getRequest() const;
			t_server_context* getServerContext() const;
			t_client_socket* getClientSocket() const;

			// Setters
			void setCgi(const char* cgi);
			void setNameScript(const std::string &nameScript);
			void setPathScript(const std::string &pathScript);
			void setEnv(char** env);
			void setRequest(const std::string &request);
			void setServerContext(t_server_context *server_context);
			void setClientSocket(t_client_socket *client_socket);

			// Methods
			void printAttributes() const;
	};
}
