
#include "Server.hpp"
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstdio> // perror
#include <cerrno>

// METODO STATIC fuera de la clase.
static void setNonBlockingOrExit(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		throw std::runtime_error("fcntl() failed");
	}
}

// CONSTRUCTOR
Server::Server(const int port, const std::string &password)
	: _port(port), _password(password), _server_fd(-1) {
	initSocket();
}

// DESTRUCTOR
Server::~Server() {
	shutdown();
}

// INICIALIZA EL SOCKET DEL SERVER, LO PONE EN ESCUCHA Y LO ANAYDE AL VECTOR _fds
void Server::initSocket() {
	struct pollfd server_pollfd;

	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_server_fd == -1) {
		throw std::runtime_error("(!) ERROR: initServer: socket()");
	}

	setNonBlockingOrExit(_server_fd);

	// LIBERA PUERTO RAPIDAMENTE
	int opt = 1;
	if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1) {
		throw std::runtime_error("(!) ERROR: initServer: setsockopt()");
	}

	// CONFIGURA STRUCT PARA EL SOCKET (asigna IP y PORT)
	std::memset(&_server_addr, 0, sizeof(_server_addr));
	_server_addr.sin_family = AF_INET;		   // tipo de domain/IP (ipv4)
	_server_addr.sin_port = htons(_port);	   // puerto de entrada (donde escucha)
	_server_addr.sin_addr.s_addr = INADDR_ANY; // rango de IPs que escuchará (ANY: todas)

	// ENLAZA socket(fd) a su struct de datos(socket <-> IP + PORT)
	if (bind(_server_fd, (struct sockaddr *)&_server_addr, sizeof(_server_addr)) == -1) {
		throw std::runtime_error("(!) ERROR: initServer: bind()");
	}

	// PONE EN ESCUCHA EL PUERTO del socket (fd, num conexiones max admitidas en SO)
	if (listen(_server_fd, SOMAXCONN) == -1) {
		throw std::runtime_error("(!) ERROR: initServer: listen()");
	}

	// AÑADO struct pollfd DEL SERVER AL VECTOR _fds, será el elemento [0]
	server_pollfd.fd = _server_fd; // fd a monitorear
	server_pollfd.events = POLLIN; // mascara de bits (indica eventos que escucha)
	server_pollfd.revents = 0;	   // mascara de bits rellenada por poll()(con los eventos ya ocurridos)
	_fds.push_back(server_pollfd); // Anyade la struct del server al vector de pollfds

	std::cout << "* Server initializing...\n* Server listening on port <" << _port << ">" << std::endl;
}

// MANEJA CONEXION AL SERVER DE LOS fd DESCONOCIDOS (NUEVOS CLIENTES)
void Server::handleNewConnection() //gestiona las conexiones entrantes una a una
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	std::cout << "[DEBUG] handleNewConnection() 1" << std::endl; // DEBUG
	int client_fd = accept(_server_fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) // si no hubo nada que aceptar, vuelve al loop principal (run)
            return;
		throw std::runtime_error("(!) ERROR: handleNewConnection(): accept()");
    }
    // POSIBLE REEMPLAZO: acepta todoas las conexiones entrantes de golpe y las va gestionando en el bucle
    // while (true)
    // {
	// 	std::cout << "handleNewConnection() 2" << std::endl; // DEBUG
    //     int client_fd = accept(_server_fd, (struct sockaddr *)&client_addr, &client_len);
    //     std::cout << "handleNewConnection() 3" << std::endl; // DEBUG
	// 	if (client_fd == -1)
    //     {
	// 		std::cout << "handleNewConnection() 4" << std::endl; // DEBUG
    //         if (errno == EAGAIN || errno == EWOULDBLOCK) {
	// 			std::cout << "handleNewConnection() 5" << std::endl; // DEBUG
    //             break; // no hay más conexiones pendientes
	// 		}
    //         throw std::runtime_error("(!) ERROR: NewConnection: accept()");
    //     }
		
		std::cout << "[DEBUG] handleNewConnection() 6" << std::endl; // DEBUG
		setNonBlockingOrExit(client_fd); // Si falla salta una exception
		
		// AÑADO struct pollfd DEL NEW CLIENT AL VECTOR _fds
		struct pollfd client_pollfd;
		client_pollfd.fd = client_fd;
		client_pollfd.events = POLLIN;
		client_pollfd.revents = 0;
		_fds.push_back(client_pollfd);

		std::cout << "[DEBUG] handleNewConnection() 7" << std::endl; // DEBUG

		// CREA OBJ Client Y LO ANYADE AL MAP _clients
		_clients[client_fd] = Client(client_fd);
		// forma alternativa de construir el obj directamente en el map
		//_clients.insert(std::pair<int, Client>(client_fd, Client(client_fd)));
		
		std::cout << "[INFO] Client connected fd <" << client_fd << ">" << std::endl;
		// OJO; PENDIENTE DE IMPLEMENTAR getAddr para obtener IP y port del client
		//	std::cout << "Client IP: " << inet_ntoa(obj_cli.getAddr().sin_addr) << std::endl;
		//	std::cout << "Client Port: " << ntohs(obj_cli.getAddr().sin_port) << std::endl;
	//	std::cout << "[DEBUG] handleNewConnection() 8" << std::endl; // DEBUG
}


Client *Server::getClient(int fd) {
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return NULL;	  // no encontrado
	return &(it->second); // retorna un puntero
}

// MANEJA CONEXIONES AL SERVER DE LOS fd CONOCIDOS (MENSAJES)
void Server::handleClientMessage(size_t i) {
	std::cout << "[DEBUG] handleClientMessage() 1" << std::endl; //DEBUG
	int fd = _fds[i].fd;
	char buffer[1024]; // pueden llegar comandos concatenados, fragmentados, ya procesaremos luego en parser segun protocol IRC.
	std::memset(buffer, 0, sizeof(buffer));

	ssize_t bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
	//=================================================

	if (bytes == 0) {
		// Cliente cerró conexión
		disconnectClient(fd);
		return;
	}
	else if (bytes < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// No hay datos disponibles, esperamos al siguiente poll()
			return;
		} else {
			// Error real de lectura
			disconnectClient(fd);
			return;
		}
	}

	//===============================================

	// if (bytes <= 0){  // cliente cerro la conexion o error de lectura
	// 	std::cout << "handleClientMessage() 2" << std::endl; // DEBUG
	// 	disconnectClient(fd); // un solo punto de desconexión
	// 	return;
	// }

	// Recuperar el cliente desde el map
	Client *cli = getClient(fd);
	if (cli == NULL)
		return;

	// Añadir los datos al buffer interno del cliente
	cli->appendToBuffer(std::string(buffer, bytes));

	// Procesar comandos completos (\r\n)
	std::string &buf = cli->getBuffer();
	size_t pos;

	while ((pos = buf.find("\r\n")) != std::string::npos)
	{
		std::cout << "[DEBUG] handleClientMessage() 3" << std::endl; // DEBUG
		std::string command = buf.substr(0, pos);
		buf.erase(0, pos + 2); // eliminar hasta \r\n

		if (!command.empty())
		{
			std::cout << "Client fd <" << fd << "> Command: "
					  << command << std::endl;

			// Delegar al parser del protocolo IRC
			processCommand(cli, command);
		}
	}
}


void Server::disconnectClient(int client_fd)
{
	// Cerrar el socket
	close(client_fd);

	// Eliminar del map de _clients
	std::map<int, Client>::iterator it = _clients.find(client_fd);
	if (it != _clients.end())
	{
		std::cout << "Client fd <" << client_fd << "> disconnected" << std::endl;
		_clients.erase(it);
	}

	// Eliminar del vector de pollfds _fds
	for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
	{
		if (it->fd == client_fd)
		{
			_fds.erase(it);
			break; // importante, sino se invalida el iterador
		}
	}
}

void Server::shutdown()
{
	// cerrar clientes
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
	}
	_clients.clear();
	_fds.clear();
	std::cout << "\n[INFO] Server shutdown gracefully" << std::endl;
}

//=============================================

// Helper para dividir un string en tokens
std::vector<std::string> split(const std::string &str, char delim)
{
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;
	while (std::getline(ss, token, delim))
		tokens.push_back(token);
	return tokens;
}

void Server::sendToClient(int fd, const std::string &msg)
{
	if (fd < 0 || msg.empty())
		return;

	ssize_t totalSent = 0;
	ssize_t msgLen = static_cast<ssize_t>(msg.size());

	while (totalSent < msgLen)
	{
		ssize_t sent = ::send(fd, msg.c_str() + totalSent, msgLen - totalSent, 0);
		if (sent > 0) {
			totalSent += sent;
		} 
		else if (sent == -1) {
			if (errno == EINTR)
				continue; // señal interrumpe, reintenta
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				// no podemos enviar ahora, el socket está lleno
				// podemos salir y esperar que poll() nos indique cuando se pueda escribir
				break;
			} 
			else {
				std::cerr << "Error sending to fd " << fd << std::endl;
				break;
			}
		}
	}

}

// HANDSHAKE: autenticar cliente
void Server::handshake(Client *cli, const std::string &cmd)
{
	std::cout << "[DEBUG] Entrando en handshake" << std::endl;
	if (cmd.empty())
		return;

	std::vector<std::string> tokens = split(cmd, ' ');
	if (tokens.empty())
		return;

	std::string command = tokens[0];
	// std::transform(command.begin(), command.end(), command.begin(), ::toupper);

	// Cliente ya autenticado → salimos
	if (cli->getStatus() == REGISTERED)
		return;
	std::cout << "[DEBUG] handshake CAP" << std::endl;
	// Procesamos comandos de autenticación
	if (command == "CAP")
	{
		handleCap(cli); //, tokens);
	}
	else if (command == "PASS")
	{
		std::cout << "[DEBUG] handshake PASS" << std::endl;
		handlePass(cli, tokens);
	}
	else if (command == "NICK")
	{
		handleNick(cli, tokens);
	}
	else if (command == "USER")
	{
		handleUser(cli, tokens);
	}
	else
	{
		std::cout << "[DEBUG] handshake -> 451" << std::endl;
		sendToClient(cli->getFd(), "451 :You have not registered\r\n");
	}

	// Si el cliente llega a USER_OK → lo marcamos como REGISTERED
	if (cli->getStatus() == USER_OK)
	{
		cli->setStatus(REGISTERED);
		sendToClient(cli->getFd(), ":server 001 " + cli->getNick() + " :Welcome to the IRC server!\r\n");
		std::cout << "Client fd=" << cli->getFd()
				  << " registered as " << cli->getNick() << std::endl;
	}
}

void Server::handleCap(Client *cli) //, const std::vector<std::string> &tokens)
{
    // Responder con lista vacía de capabilities
    std::ostringstream reply;
    std::string nick = cli->getNick().empty() ? "nickname" : cli->getNick();

    reply << ":Server CAP " 
          << nick 
          << " LS :\r\n";

    sendToClient(cli->getFd(), reply.str());
}


void Server::handlePass(Client *cli, const std::vector<std::string> &tokens)
{
	if (tokens.size() < 2)
	{
		sendToClient(cli->getFd(), "461 PASS :Not enough parameters\r\n");
		return;
	}

	if (tokens[1] == this->_password)
	{
		cli->setStatus(PASS_OK);
		std::cout << "PASS accepted" << std::endl;
	}
	else
	{
		sendToClient(cli->getFd(), "464 :Password incorrect\r\n");
	}
}

void Server::handleNick(Client *cli, const std::vector<std::string> &tokens)
{
	if (tokens.size() < 2)
	{
		sendToClient(cli->getFd(), "431 :No nickname given\r\n");
		return;
	}

	// No se si deberiamos validar si el NICK ya existe ??????

	cli->setNick(tokens[1]);

	if (cli->getStatus() == PASS_OK)
		cli->setStatus(NICK_OK);

	std::cout << "NICK set to " << tokens[1] << std::endl;
}

void Server::handleUser(Client *cli, const std::vector<std::string> &tokens)
{
	if (tokens.size() < 2) // OJO hay 4 tokens !!!!!!
	{
		sendToClient(cli->getFd(), "461 USER :Not enough parameters\r\n");
		return;
	}

	cli->setUser(tokens[1]);

	if (cli->getStatus() == NICK_OK)
	{
		cli->setStatus(USER_OK);
	}

	std::cout << "USER set to " << tokens[1] << std::endl;
}

// PARSE COMMAND: clientes ya autenticados
void Server::processCommand(Client *cli, const std::string &cmd)
{
	if (cli->getStatus() != REGISTERED)
	{
		handshake(cli, cmd); // si no está logueado, pasamos por el handshake
		return;
	}

	if (cmd.empty())
		return;

	std::vector<std::string> tokens = split(cmd, ' ');
	if (tokens.empty())
		return;

	std::string command = tokens[0];
	// std::transform(command.begin(), command.end(), command.begin(), ::toupper);

	if (command == "JOIN")
	{
		//    handleJoin(cli, tokens);
	}
	else if (command == "PRIVMSG")
	{
		//    handlePrivmsg(cli, tokens);
	}
	else
	{
		sendToClient(cli->getFd(), "421 " + command + " :Unknown command\r\n");
	}
}

//=====================================================

// ESCUCHA PERMANENTE DE ACTIVIDAD CON poll()
// si una señal externa (Ctrl+C) despierta a poll(), va al inicio
// del bucle y sale de la funcion aunque no haya actividad en los sockets
void Server::run()
{
	std::cout << "[DEBUG] entra run() por primera vez" << std::endl;
	while (Server::_signalFlag == false)
	{
		std::cout << "[DEBUG] entra en while de run()" << std::endl;
		int activity = poll(&_fds[0], _fds.size(), -1); // -1 espera indefinidamente
		if (activity < 0)
		{ //Discrimina si sale por Ctrl+C o por error en poll()
			if (errno == EINTR) // retorno de poll() si recibe una SIGINT
				continue;
			throw std::runtime_error("Error en poll()");
		}
//		sleep(1); // DEBUG
		for (size_t i = 0; i < _fds.size(); ++i)
		{
			// if (_fds[i].revents != 0) {
			// 	std::cout << "[DEBUG] FD " << _fds[i].fd 
			// 			  << " tiene evento revents=" << _fds[i].revents << "\n";
			// }
			if (_fds[i].revents & POLLIN)
			{
				if (_fds[i].fd == _server_fd)
				{ // actividad en el fd del server
					std::cout << "[DEBUG] hubo un POLLIN en el server_fd de run()" << std::endl;
					handleNewConnection();
				}
				else
				{ // actividad en algun fd de cliente
					std::cout << "[DEBUG] hubo un POLLIN en un client_fd de run()" << std::endl;
					handleClientMessage(i);
				}
			}
//DEBBUG=========================
			if (_fds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
				if (_fds[i].revents & POLLHUP) std::cout << "POLLHUP detected\n";
				if (_fds[i].revents & POLLERR) std::cout << "POLLERR detected\n";
				if (_fds[i].revents & POLLNVAL) std::cout << "POLLNVAL detected\n";

				std::cout << "[INFO] Cliente desconectado fd <"
						  << _fds[i].fd << ">\n";
				close(_fds[i].fd);
				_fds.erase(_fds.begin() + i);
				i--; // importante para no saltarse el siguiente
			}
//=======================================			
		}
	}
}

// SIGNALS
// Definicion e inicializacion, fuera de la clase.
bool Server::_signalFlag = false;

// Setea a true la flag de signals
void Server::signalHandler(int signum)
{
	(void)signum;
	//    std::cout << std::endl << "Signal Received!" << std::endl;
	Server::_signalFlag = true;
}