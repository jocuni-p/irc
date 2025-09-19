#include "Server.hpp"


bool Server::_signalFlag = false;

//We use this as a flag to stop the process in a clean way
void Server::signalHandler(int signum)
{
    (void)signum;
    Server::_signalFlag = true;
}

// Constructor
Server::Server() : _port(0), _serverFd(-1) {}


//CREACION SOCKET DEL SERVER
void Server::createSocket()
{
	int en = 1;
	struct sockaddr_in add;
	struct pollfd newPoll;

	// INICIALIZA STRUCT DEL SOCKET (asigna IP, PORT y escucha a cualquier ip))
	add.sin_family = AF_INET;
	add.sin_addr.s_addr = INADDR_ANY;
	add.sin_port = htons(this->_port);

	// CREA EL SOCKET
	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverFd == -1)
		throw(std::runtime_error("faild to create socket"));

	// CONF LIBERACION RAPIDA DEL PUERTO
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1)
		throw(std::runtime_error("faild to set option (SO_REUSEADDR) on socket"));

	// HACE NO BLOQUEANTE AL SOCKET
	if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) == -1)
		throw(std::runtime_error("faild to set option (O_NONBLOCK) on socket"));

	// ENLAZA el socket a su struct de datos(socket <-> IP + PORT)
	if (bind(_serverFd, (struct sockaddr *)&add, sizeof(add)) == -1)
		throw(std::runtime_error("faild to bind socket"));

	// PONE EN ESCUCHA EL PUERTO del socket (fd, num conexiones max admitidas en SO)
	if (listen(_serverFd, SOMAXCONN) == -1)
		throw(std::runtime_error("listen() on server failed"));

	// CONFIG y AÑADE struct pollfd DEL SERVER al vector<> _fds
	newPoll.fd = _serverFd;
	newPoll.events = POLLIN;
	newPoll.revents = 0;
	_fds.push_back(newPoll);

}
// CREA SERVER Y LO PONE EN BUCLE DE ESCUCHA
void Server::serverInit(int port, std::string& pwd)
{
	this->_port = port;
	this->_password = pwd;
	createSocket();
	
	std::cout << GREEN << "\n** Server IRC created **" << RESET << std::endl;
	std::cout << "Listening on port <" << this->_port << ">\n" << std::endl;

	//NOTA: para evitar malfuncionamiento no borraremos ningun cliente 
	//mientras el bucle poll() todavía pueda acceder a él, sino que lo
	//marcaremos (flag) como pendiente de eliminacion y al salir del bucle
	// se eliminara de forma segura.
	while (Server::_signalFlag == false)
	{
		if ((poll(&_fds[0], _fds.size(), -1) == -1) && Server::_signalFlag == false)
			throw(std::runtime_error("poll() failed"));

		for (int i = static_cast<int>(_fds.size()) - 1; i >= 0; --i)
		{
			if (!(_fds[i].revents & POLLIN))
				continue;
			
			if (_fds[i].fd == _serverFd)
				acceptNewClient();
			else
				receiveNewData(_fds[i].fd);
		}
		// Limpia clientes marcados para borrado (al reves, por seguridad)
		for (int i = static_cast<int>(_clients.size()) - 1; i >= 0; --i) {
			if (_clients[i].isMarkedForRemoval()) {
				int fd = _clients[i].getFd();

				// Lo busca y lo elimina de _fds
				for (size_t j = 0; j < _fds.size(); ++j) {
					if (_fds[j].fd == fd) {
						_fds.erase(_fds.begin() + j);
						break;
					}
				}

				// Lo elimina de todos los canales
				for (size_t c = 0; c < _channels.size();  ) { // <--incremento manual
					_channels[c].removeClient(fd); //solo borra si lo encuentra

					// si es el unico en el canal, borra tambien el canal
					if (_channels[c].getClients().empty()) {
						_channels.erase(_channels.begin() + c);
						continue; // no incrementa c porque el vector se ha desplazado
					}
					++c;
				}

				//Lo elimina de _clients
				_clients.erase(_clients.begin() + i);
			}
		}
		// printClients(_clients); // DEBUG
		// printChannels(_channels); // DEBUG
	}
	closeFds(); 
}





//Cierra socket limpiamente, marca cliente para ser borrado, y lo informa en consola
void Server::clearClient(int fd)
{
    Client* cli = getClient(fd);
    if (!cli) return;

    cli->markForRemoval(); // lo marca con 'true' para borrado
    shutdown(fd, SHUT_RDWR);
    close(fd);

    std::cout << YELLOW_PALE << "<" << fd << "> Disconnected" << RESET << std::endl;
}




void Server::closeFds()
{
    // Cierra clientes (forma limpia)
    for (size_t i = 0; i < _clients.size(); ++i) {
        int cfd = _clients[i].getFd();
        if (cfd >= 0) {
            std::cout << YELLOW_PALE << "<" << cfd << "> Disconnected" << RESET << std::endl;
            shutdown(cfd, SHUT_RDWR); // avisa del cierre del socket, forma mas limpia
            close(cfd); //cierra y libera el fd
        }
    }

    // Cierra el socket del servidor
    if (_serverFd != -1) {
        std::cout << YELLOW_PALE << "Server Disconnected" << RESET << std::endl;
        shutdown(_serverFd, SHUT_RDWR);
        close(_serverFd);
        _serverFd = -1;
    }

    // Trick para limpiar estructuras (C++98)
	//Creamos un vector temporal vacio de tipo Client. 
	//Con swap() intercambiamos el contenido del vector temporal
	// vacio con _clients. Ahora _clients esta vacio. El vector 
	//temporal (que contiene los datos antiguos) se destruye al 
	//salir de la expresion → liberando su memoria.
	//No es necesario recorrer los vectores para “cerrar” nada 
	//porque los no tienen sockets propios. Solo contienen sets,
	//bools, int y strings, que se liberan automaticamente al 
	//destruirse los objetos.
    std::vector<Client>().swap(_clients);
    std::vector<struct pollfd>().swap(_fds);
	std::vector<Channel>().swap(_channels);

	std::cout << GREEN << "** IRC Server shutdown gracefully **" << RESET << std::endl;
}







void Server::parseCommand(Client* cli, const std::string& cmd)
{
	if (cmd.empty())
		return ;

	if (cli->getStatus() != AUTHENTICATED)
	{
		handshake(cli, cmd);
		return;
	}

	// Separa en tokens por espacio
	std::vector<std::string> tokens = Utils::split(cmd, ' ');
	if (tokens.empty())
		return ;

	std::string command = tokens[0];

	for (size_t i = 0; i < command.size(); ++i) {
		command[i] = std::toupper(command[i]);
	}

	if (command == "CAP" || command == "CAP END") { // ignora estos comandos si vuelven de nuevo
		return ;
	}
	else if (command == "PASS") { // No se permite cambio de PASS
		handlePass(cli, tokens);
	}
	else if (command == "NICK") { // Permite cambio de NICK
		handleNick(cli, tokens);
	}
	else if (command == "USER") {
		return;
	}
	else if (command == "JOIN")
	{
		handleJoin(cli, tokens);
	}
	else if (command == "WHO") { //Envia la lista de clientes del canal
		handleWho(cli, tokens);
	}
	else if (command == "PRIVMSG") {
		handlePrivmsg(cli, tokens);
	}
	else if (command == "MODE") {
		handleMode(cli, tokens);
	}
	else if (command == "KICK") {
		handleKick(cli, tokens);
	}
	else if (command == "TOPIC") {
		handleTopic(cli, tokens);
	}
	else if (command == "INVITE") {
		handleInvite(cli, tokens);
	}
	else if (command == "QUIT") { //Maneja una eliminacion y borrado limpio del cliente
	 	clearClient(cli->getFd()); //cierra socket, lo elimina de _fds, _clients y _channels y lo informa en consola 
	}
	else {
		std::cout << "Ignored command by ircserv: " << cmd << std::endl;
		//Aqui llegaran los comandos no implementados por subject
	}
}




