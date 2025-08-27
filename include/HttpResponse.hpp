#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

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

class HttpResponse {

	private:
		std::string version_;
		int status_code_;
		std::string status_message_;
		std::map<std::string, std::string> headers_;
		std::string body_;

	public:
		HttpResponse();
		//HttpResponse(const std::string& version, int status_code, const std::string& status_message);
		HttpResponse(const HttpResponse& other);
		HttpResponse& operator=(const HttpResponse& other);
		~HttpResponse();

		const std::string& getVersion() const;
		int getStatusCode() const;
		const std::string& getStatusMessage() const;
		const std::map<std::string, std::string>& getHeaders() const;
		const std::string& getBody() const;

		void setVersion(const std::string& version);
		void setStatusCode(int code);
		void setStatusMessage(const std::string& message);
		void setHeaders(const std::map<std::string, std::string>& headers);
		void setBody(const std::string& body);

		std::string buildResponse() const;
};

#endif
