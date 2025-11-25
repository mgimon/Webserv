#include "../include/CGIHandler.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include "../include/utilsCC.hpp"
void sendInternalError(int client_socket_fd, std::map<int, t_fd_data> &map_fds)
{
	HttpResponse http_response;

	std::map<int, t_fd_data>::iterator fds_it = map_fds.find(client_socket_fd);
	t_client_socket *s_client_socket = static_cast<t_client_socket*>(fds_it->second.data);
	http_response.setError(utils::getErrorPath(s_client_socket->server, 500), 500, "Internal Server Error");
	http_response.respondInClient(client_socket_fd);
}

static inline void trim(std::string &s) {
	while (!s.empty()) {
		char c = s[s.size() - 1];
		if (c == '\r' || c == ' ' || c == '\t')
			s.erase(s.size() - 1);
		else
			break;
	}
	size_t i = 0;
	while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
	if (i) s.erase(0, i);
}

bool parse_cgi_headers(const std::string &buf, size_t &headers_end, std::map<std::string,std::string> &headers, int &status_code, std::string &status_text) {
    size_t pos = std::string::npos;
    // Prefer \r\n\r\n, fallback a \n\n
    pos = buf.find("\r\n\r\n");
    size_t sep_len = 4;
    if (pos == std::string::npos) {
        pos = buf.find("\n\n");
        sep_len = 2;
        if (pos == std::string::npos) return false; // aún no llegaron headers completos
    }
    headers_end = pos + sep_len;
    std::string header_block = buf.substr(0, pos + 1); // +1 para asegurar terminar la última línea en getline

    std::istringstream ss(header_block);
    std::string line;
    bool first_line = true;
    status_code = 200;
    status_text = "OK";

	while (std::getline(ss, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        if (line.empty()) break;
        if (first_line) {
            first_line = false;
            // Detect HTTP status line: "HTTP/1.1 200 OK"
            if (line.find("HTTP/") == 0) {
                std::istringstream ls(line);
                std::string httpver;
                ls >> httpver >> status_code;
                std::getline(ls, status_text);
                trim(status_text);
                continue;
            }
        }
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);
        trim(key);
        trim(val);
        // lowercase the key
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        headers[key] = val;
    }

    // If there's a Status header, it overrides
    if (headers.count("status")) {
        std::istringstream ss2(headers["status"]);
        ss2 >> status_code;
        std::getline(ss2, status_text);
        trim(status_text);
    } else if (headers.count("location") && status_code == 200) {
        // Redirect without explicit Status -> default to 302
        status_code = 302;
        status_text = "Found";
    }

    return true;
}

void CGIHandler::monitor(int epoll_fd, std::map<int, t_fd_data> &map_fds, std::map<pid_t, t_pid_context> &map_pids)
{
	std::map<pid_t, t_pid_context>::iterator pids_it = map_pids.begin();
	while (pids_it != map_pids.end())
	{
		if (pids_it->second.time >= 50)
		{
			//std::cerr << RED << "CGI closed by TimeOut" << RESET << std::endl;
			kill(pids_it->first, SIGKILL);
			sendInternalError(pids_it->second.client_socket_fd, map_fds);
			UtilsCC::cleanCGI(epoll_fd, pids_it, map_fds);
			std::map<pid_t, t_pid_context>::iterator aux_it = pids_it;
			++pids_it;
			map_pids.erase(aux_it);
		}
		else
		{
			pids_it->second.time++;
			++pids_it;
		}
	}
}

void CGIHandler::writeInPipe(t_CGI_pipe_write *s_pipe_write, uint32_t event, t_server_context &server_context)
{
	std::map<pid_t, t_pid_context>::iterator pids_it = server_context.map_pids.find(s_pipe_write->pid);

	//Si la pipe se ha cerrado antes de acabar de escribir limpiamos
	if (event & (EPOLLHUP | EPOLLERR))
	{
		//std::cerr << RED << "CGI closed by error at writePipe" << RESET << std::endl;
		kill(s_pipe_write->pid, SIGKILL);
		sendInternalError(s_pipe_write->client_socket->socket_fd, server_context.map_fds);
		UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds);
		server_context.map_pids.erase(pids_it);
		return;
	}
	pids_it->second.time = 0;
	//Calcular cuanto vamos a enviar
	size_t remaining = s_pipe_write->content_length - s_pipe_write->sended;
	size_t send_length = (remaining >= 4096) ? 4096 : remaining;
	//Calcular a partir de donde hemos de escribir
	const void *pos = s_pipe_write->request_body.c_str() + s_pipe_write->sended;

	ssize_t bytesSend = write(s_pipe_write->fd, pos, send_length);
	if (bytesSend <= 0)
		return;
	
	// Si se ha escrito incrementamos el contador de bytes enviados
	s_pipe_write->sended += bytesSend;
	if (s_pipe_write->sended >= s_pipe_write->content_length)
	{
		pids_it->second.write_finished = true;
		epoll_ctl(server_context.epoll_fd, EPOLL_CTL_DEL, s_pipe_write->fd, NULL);
		close(s_pipe_write->fd);
		server_context.map_fds.erase(s_pipe_write->fd);
		delete(s_pipe_write);
	}
	
	// NOTA IMPORTANTE: No se que pasa si el content length es 0, no se si es posible y no lo he controlado, 
	// mirar por si acaso. Tambien, en este codigo se da por hecho que content-length es el tamano del body limpio.
	// Confirmar que es asi lo del content-length
}

void CGIHandler::readFromPipe(t_CGI_pipe_read *s_pipe_read, uint32_t event, t_server_context &server_context)
{
	std::map<pid_t, t_pid_context>::iterator pids_it = server_context.map_pids.find(s_pipe_read->pid);

	//Si la pipe se ha cerrado antes de acabar de leer limpiamos
	if (event & (EPOLLHUP | EPOLLERR))
	{
		//std::cerr << RED << "CGI closed by error at readPipe" << RESET << std::endl;
		kill(s_pipe_read->pid, SIGKILL);
		sendInternalError(s_pipe_read->client_socket->socket_fd, server_context.map_fds);
		UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds);
		server_context.map_pids.erase(pids_it);
		return;
	}
	pids_it->second.time = 0;
	
	char buf[4096];
    ssize_t bytesRead = read(s_pipe_read->fd, buf, sizeof(buf));

    if (bytesRead < 0)
		return;
	s_pipe_read->raw_buffer.append(buf, bytesRead);

	// Intentamos parsear headers si todavía no lo hemos hecho
	size_t headers_end = 0;
	std::map<std::string, std::string> headers;
	int status_code = 200;
	std::string status_text = "OK";

	bool have_headers = parse_cgi_headers(s_pipe_read->raw_buffer, headers_end, headers, status_code, status_text);

	// Si no tenemos headers completos aún y no llegó EOF, esperamos más datos
	if (!have_headers && bytesRead > 0)
		return;

	// Si no hay headers y llegamos a EOF -> error lógico (CGI terminó sin enviar headers)
	if (!have_headers && bytesRead == 0)
	{
		kill(s_pipe_read->pid, SIGKILL);
		sendInternalError(s_pipe_read->client_socket->socket_fd, server_context.map_fds);
		UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds);
		server_context.map_pids.erase(pids_it);
		return;
	}

	// Determinar si el body está completo
	size_t content_length = 0;
	bool has_content_length = false;
	if (headers.count("content-length"))
	{
		std::istringstream iss(headers["content-length"]);
		unsigned long tmp = 0;
		if (iss >> tmp) {
			content_length = static_cast<size_t>(tmp);
			has_content_length = true;
		} else {
			has_content_length = false;
		}
	}

	size_t body_available = s_pipe_read->raw_buffer.size() - headers_end;
	bool body_complete = false;

	if (has_content_length)
		body_complete = (body_available >= content_length);
	else
		body_complete = (bytesRead == 0); // si no hay content-length, esperamos al EOF

	if (!body_complete)
		return; // esperamos más datos

	// Extraer body
	std::string body;
	if (has_content_length)
		body = s_pipe_read->raw_buffer.substr(headers_end, content_length);
	else
		body = s_pipe_read->raw_buffer.substr(headers_end);

	// Construir respuesta HTTP usando HttpResponse
	HttpResponse http_response;
	http_response.setStatusCode(status_code);
	http_response.setStatusMessage(status_text);
	http_response.setHeaders(headers);
	http_response.setBody(body);

	// Enviar al cliente
	http_response.respondInClient(s_pipe_read->client_socket->socket_fd);

	// Limpiar recursos del CGI
	UtilsCC::cleanCGI(server_context.epoll_fd, pids_it, server_context.map_fds);
	server_context.map_pids.erase(pids_it);
}
