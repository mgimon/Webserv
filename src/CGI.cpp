#include "../include/CGI.hpp"

namespace CGI
{

Handler::Handler() : cgi_(""), nameScript_(""), pathScript_(""), env_(NULL), request_(), server_context_(), client_socket_(NULL), chunked_(false) {}
Handler::Handler(const Handler& other) : cgi_(other.cgi_), nameScript_(other.nameScript_), pathScript_(other.pathScript_), env_(other.env_), request_(other.request_), server_context_(other.server_context_), client_socket_(other.client_socket_), chunked_(other.chunked_), request_body_(other.request_body_) {}
Handler& Handler::operator=(const Handler& other) { if (this != &other) { cgi_ = other.cgi_; nameScript_ = other.nameScript_; pathScript_ = other.pathScript_; env_ = other.env_; request_ = other.request_; server_context_ = other.server_context_; client_socket_ = other.client_socket_; chunked_ = other.chunked_; request_body_ = other.request_body_; } return *this; }
Handler::~Handler() {}

std::string Handler::getCgi() const { return cgi_; }
std::string Handler::getNameScript() const { return nameScript_; }
std::string Handler::getPathScript() const { return pathScript_; }
char** Handler::getEnv() const { return env_; }
std::string& Handler::getRequest() const { return const_cast<std::string&>(request_); }
t_server_context* Handler::getServerContext() const { return server_context_; }
t_client_socket* Handler::getClientSocket() const { return client_socket_; }
bool Handler::isChunked() const { return chunked_; }
std::string Handler::getRequestBody() const { return request_body_; }

void Handler::setCgi(const std::string &cgi) { cgi_ = cgi; }
void Handler::setNameScript(const std::string &nameScript) { nameScript_ = nameScript; }
void Handler::setPathScript(const std::string &pathScript) { pathScript_ = pathScript; }
void Handler::setEnv(char** env) { env_ = env; }
void Handler::setRequest(const std::string &request) { request_ = request; }
void Handler::setServerContext(t_server_context *server_context) { server_context_ = server_context; }
void Handler::setClientSocket(t_client_socket *client_socket) { client_socket_ = client_socket; }
void Handler::setRequestBody(const std::string &requestBody) { request_body_ = requestBody; }
void Handler::setChunked(const std::map<std::string, std::string> &headers)
{
    chunked_ = false;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        std::string key = it->first;
        std::string value = it->second;

		std::transform(key.begin(), key.end(), key.begin(), utils::toLowerChar);
		std::transform(value.begin(), value.end(), value.begin(), utils::toLowerChar);

        if (key == "transfer-encoding" && value.find("chunked") != std::string::npos)
        {
            chunked_ = true;
            break;
        }
    }
}

void Handler::printAttributes() const
{
    std::cout << PINK;
    std::cout << "=== CGI Handler Attributes ===" << std::endl;
    std::cout << "CGI: ";
    if (!cgi_.empty())
        std::cout << cgi_ << std::endl;
    else
        std::cout << "(null)" << std::endl;
    std::cout << "Name Script: "
              << (nameScript_.empty() ? "(empty)" : nameScript_) 
              << std::endl;
    std::cout << "Path Script: "
              << (pathScript_.empty() ? "(empty)" : pathScript_) 
              << std::endl;
    std::cout << "Environment Variables: ";
    if (env_)
	{
        std::cout << std::endl;
        for (int i = 0; env_[i]; ++i)
            std::cout << "  [" << i << "]: " << env_[i] << std::endl;
    }
	else
        std::cout << "(null)" << std::endl;
    std::cout << "Request: "
              << (request_.empty() ? "(empty)" : request_)
              << std::endl;

	std::cout << std::boolalpha;
    std::cout << "Chunked: " << chunked_ << std::endl;
    std::cout << std::noboolalpha;

    std::cout << "Server Context: " << server_context_;
    if (!server_context_) std::cout << " (null)";
    std::cout << std::endl;
    std::cout << "Client Socket: " << client_socket_;
    if (!client_socket_) std::cout << " (null)";

	std::cout << std::endl;
    std::cout << "Request Body: " << request_body_;

	std::cout << std::endl;
    std::cout << "===============================" << std::endl;
    std::cout << RESET << std::endl;
}

static void cleanReadPipe(int pipe_read_fd, t_server_context *server_context)
{
	t_fd_data pipe_read_data = server_context->map_fds.find(pipe_read_fd)->second;
	epoll_ctl(server_context->epoll_fd, EPOLL_CTL_DEL, pipe_read_fd, NULL);
	close(pipe_read_fd);
	delete(static_cast<t_CGI_pipe_read*>(pipe_read_data.data));
	server_context->map_fds.erase(pipe_read_fd);
}

static int addPipeWrite(int pipe_write_fd, int pipe_read_fd, pid_t pid, std::string request_body,
			t_client_socket *client_socket, t_server_context *server_context)
{
	epoll_event ev;
	s_CGI_pipe_write *s_pipe_write = NULL;
	bool epoll_inserted = false;
	bool data_inserted = false;

	try
	{
		s_pipe_write = new s_CGI_pipe_write(pipe_write_fd, request_body, request_body.size(), pid, client_socket);
		t_fd_data pipe_write_data = {s_pipe_write, CGI_PIPE_WRITE};
		
		ev.events = EPOLLOUT | EPOLLHUP | EPOLLERR;
		ev.data.fd = pipe_write_fd;
		if (epoll_ctl(server_context->epoll_fd, EPOLL_CTL_ADD, pipe_write_fd, &ev) == -1)
			throw std::runtime_error(strerror(errno));
		epoll_inserted = true;
		data_inserted = server_context->map_fds.insert(std::make_pair(pipe_write_fd, pipe_write_data)).second;
		
		t_pid_context pid_context = {time(NULL), pipe_write_fd, pipe_read_fd, client_socket->socket_fd, false};
		server_context->map_pids.insert(std::make_pair(pid, pid_context));
	}
	catch(const std::exception& e)
	{
		std::cerr << RED << e.what() << RESET << std::endl;
		//Liberamos el read pipe
		cleanReadPipe(pipe_read_fd, server_context);
		//Liberamos el write pipe
		if (epoll_inserted)
			epoll_ctl(server_context->epoll_fd, EPOLL_CTL_DEL, pipe_read_fd, NULL);
		close(pipe_write_fd);
		if (s_pipe_write != NULL)
			delete(s_pipe_write);
		if (data_inserted)	
			server_context->map_fds.erase(pipe_write_fd);
		kill(pid, SIGKILL); // Cerramos el proceso hijo
		return (0);
	}
	return(1);
}

static int addPipeRead(int pipe_write_fd, int pipe_read_fd, pid_t pid, t_client_socket *client_socket, t_server_context *server_context)
{
	epoll_event ev;
	s_CGI_pipe_read *s_pipe_read = NULL;
	bool epoll_inserted = false;

	try
	{
		s_pipe_read = new s_CGI_pipe_read(pipe_read_fd, pid, client_socket);
		t_fd_data pipe_read_data = {s_pipe_read, CGI_PIPE_READ};
		ev.events = EPOLLIN | EPOLLERR;
		ev.data.fd = pipe_read_fd;
		if (epoll_ctl(server_context->epoll_fd, EPOLL_CTL_ADD, pipe_read_fd, &ev) == -1)
			throw std::runtime_error(strerror(errno));
		epoll_inserted = true;
		server_context->map_fds.insert(std::make_pair(pipe_read_fd, pipe_read_data));
	}
	catch(const std::exception& e)
	{
		std::cerr << RED << e.what() << RESET << std::endl;
		//Liberamos el read pipe y cerrramos el fd de write pipes
		close(pipe_write_fd);
		if (epoll_inserted)
			epoll_ctl(server_context->epoll_fd, EPOLL_CTL_DEL, pipe_read_fd, NULL);
		close(pipe_read_fd);
		if (s_pipe_read != NULL)
			delete(s_pipe_read);
		kill(pid, SIGKILL); //Cerramos el proceso hijo
		return (0);
	}
	return(1);
}

static void execCGI(int *pipe_write, int *pipe_read, const std::string &cgi, const std::string &nameScript, const std::string &pathScript, char **env)
{	
	char *argv[3];

	dup2(pipe_write[0], STDIN_FILENO); // Creo un fd de la tuberia con el numero 0 (Standar Input)
	dup2(pipe_read[1], STDOUT_FILENO); // Creo un fd de la tuberia con el numero 1 (Standar Output);  
	//Cerramos los fds que acabamos de duplicar
	close(pipe_write[0]);
	close(pipe_read[1]);
	//Cerramos los fds de los otros exetremo de las tuberias que no necesitamos en el hijo
	close(pipe_write[1]);
	close(pipe_read[0]);
	
	if (pathScript != "")
	{
		if (chdir(pathScript.c_str()) == -1)
		{
			std::cerr << RED << "Error chdir in child process" << RESET << std::endl;
			while(1);
		}
	}
	//Creamos el array del comando
	argv[0] = const_cast<char*>(cgi.c_str());
	argv[1] = const_cast<char*>(nameScript.c_str());
	argv[2] = NULL;
	//Ejecutamos
	execve(argv[0], argv, env);
	std::cerr << RED << "Error execv" << RESET << std::endl;
	while(1);
}

static void closePipe(int *pipe)
{
	close(pipe[0]);
	close(pipe[1]);
}

static int configPipes(int *pipe_write, int *pipe_read)
{
	if (pipe(pipe_write) == -1)
		return (0); 
	if (pipe(pipe_read))
	{
		closePipe(pipe_write);
		return (0); 
	}
	if (!UtilsCC::addFlagsFd(pipe_write[1]) || !UtilsCC::addFlagsFd(pipe_read[0]))
	{
		closePipe(pipe_write);
		closePipe(pipe_read);
		return (0); 
	}
	return(1);
}

int Handler::startCGI()
{
	int pipe_write[2]; // Padre escribe, hijo lee
	int pipe_read[2]; // Padre lee, hijo escribe

	if (configPipes(pipe_write, pipe_read) == 0)
		return(0);
	pid_t pid = fork();
	if (pid == -1)
	{
		closePipe(pipe_write);
		closePipe(pipe_read);
		return (0);
	}
	if (pid == 0) // Child process
		execCGI(pipe_write, pipe_read, cgi_, nameScript_, pathScript_, env_);
	//Cerramos los fds de los otros extremo de las tuberias que no necesitamos en el padre
	close(pipe_write[0]);
	close(pipe_read[1]);
	
	if (addPipeRead(pipe_write[1], pipe_read[0], pid, client_socket_, server_context_) == 0)
		return (0);

	//Si el metodo es POST, usamos las dos pipes, si no cerramos la de lectura
	if (request_ == "POST")
	{
		if (addPipeWrite(pipe_write[1], pipe_read[0], pid, request_body_, client_socket_, server_context_) == 0)
			return(0);
	}
	else
	{
		close(pipe_write[1]);
		try
		{
			t_pid_context pid_context = {time(NULL), -1, pipe_read[0], client_socket_->socket_fd, true};
			server_context_->map_pids.insert(std::make_pair(pid, pid_context));
		}
		catch(const std::exception& e)
		{
			std::cerr << RED << e.what() << RESET << std::endl;
			//Liberamos el read pipe
			cleanReadPipe(pipe_read[0], server_context_);
			kill(pid, SIGKILL); // Cerramos el proceso hijo
			return (0);
		}
	}
	client_socket_->cgi = true;
	client_socket_->pid = pid;
	return(1);
}

}
