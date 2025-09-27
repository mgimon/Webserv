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

// Forward declarations
class ServerConfig;
namespace utils {
    std::string getErrorPath(ServerConfig &serverOne, int errcode);
}

class HttpResponse {

	private:
		std::string version_;
		int status_code_;
		std::string status_message_;
		std::map<std::string, std::string> headers_;
		std::string body_;

		std::string content_type_;

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

		void setContentType(const std::string &path);

		void setError(const std::string &filepath, int status, const std::string &error_msg);
		void set200(std::ifstream &file);

		void buildResponse(std::string path, ServerConfig &serverOne);
		void forceConnectionClose();
		void respondInClient(int client_fd);
};

/* 

about.html → pagina informativa

form.html → probar POST con datos simples

upload.html → probar subida de archivos

delete.html → probar DELETE sobre ficheros

tests.html → hub de pruebas con enlaces a todas las paginas

TODO: Un redirect.html con un link a una ruta configurada en config como HTTP redirect
TODO: Un HTML con un formulario que haga POST a un .php o .py (ej. cgi-bin/test.php) para probar ejecucion de CGI

*/

#endif
