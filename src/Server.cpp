#include "Server.hpp"


bool Server::_signalFlag = false;

void Server::signalHandler(int signum)
{
    (void)signum;
//    std::cout << std::endl << "Signal Received!" << std::endl;
    Server::_signalFlag = true;
}

// CONSTRUCTOR POR DEFECTO
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

	//NOTA: no borrar un cliente mientras tu bucle poll() todavía pueda
	// acceder a él, sino marcarlo como pendiente de cierre y eliminarlo 
	//de forma segura después.
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
		// Limpia clientes marcados para borrado
		for (int i = static_cast<int>(_clients.size()) - 1; i >= 0; --i) {
			if (_clients[i].isMarkedForRemoval()) {
				int fd = _clients[i].getFd();

				// Buscarlo y eliminarlo de _fds
				for (size_t j = 0; j < _fds.size(); ++j) {
					if (_fds[j].fd == fd) {
						_fds.erase(_fds.begin() + j);
						break;
					}
				}

				// Eliminarlo de todos los canales
				for (size_t c = 0; c < _channels.size();  ) { // <--incremento manual
					_channels[c].removeClient(fd); //solo borra si lo encuentra

					// si es el unico en el canal, borra tambien el canal
					if (_channels[c].getClients().empty()) {
						_channels.erase(_channels.begin() + c);
						continue; // no incrementa c porque el vector se ha desplazado
					}
					++c;
				}

				//Eliminarlo de _clients
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
            shutdown(cfd, SHUT_RDWR); // aviso y cierre de socket, forma mas limpia
            close(cfd); //libera el fd
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
	//Creamos un vector temporal vacío de tipo Client. 
	//Con swap() intercambiamos el contenido del vector temporal
	// vacío con _clients. Ahora _clients está vacío. El vector 
	//temporal (que contiene los datos antiguos) se destruye al 
	//salir de la expresión → liberando su memoria.
	//No es necesario recorrer los vectores para “cerrar” nada 
	//porque los no tienen sockets propios. Sólo contienen sets,
	//bools, int y strings, que se liberan automáticamente al 
	//destruirse los objetos.
    std::vector<Client>().swap(_clients);
    std::vector<struct pollfd>().swap(_fds);
	std::vector<Channel>().swap(_channels);

	std::cout << GREEN << "** IRC Server shutdown gracefully **" << RESET << std::endl;
}



void Server::acceptNewClient()
{
    struct sockaddr_in cliAdd;
    socklen_t len = sizeof(cliAdd);
    int newFd = accept(_serverFd, (sockaddr *)&cliAdd, &len);
    if (newFd == -1) { // si accept da -1 => no hay nada ahora, pues salgo de la funcion
        if (errno == EAGAIN || errno == EWOULDBLOCK) return; 
		std::perror("(!) ERROR:  accept()");
        return; // no continua con fd invalido
//		throw std::runtime_error("(!) ERROR:  accept()"); ??? NO SE SI LANZAR EL PERROR O LA EXCEPTION
    }

    if (fcntl(newFd, F_SETFL, O_NONBLOCK) == -1) {
        ::close(newFd);
		throw std::runtime_error("(!) ERROR: fcntl(O_NONBLOCK)");
    }

    struct pollfd newPoll;
    newPoll.fd = newFd;
    newPoll.events = POLLIN;
    newPoll.revents = 0;

    Client cli;
    cli.setFd(newFd);
    cli.setIp(inet_ntoa(cliAdd.sin_addr));

    _clients.push_back(cli);
    _fds.push_back(newPoll);

	std::cout << YELLOW_PALE << "<" << newFd << "> Connected" << RESET << std::endl;
}



Client *Server::getClient(int fd)
{
	for (size_t i = 0; i < _clients.size(); i++)
    {
		if (_clients[i].getFd() == fd)
			return &_clients[i];
	}
	return NULL;
}

void Server::receiveNewData(int fd)
{
    char buff[1024];
    memset(buff, 0, sizeof(buff));

    ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);

    if (bytes <= 0)
    {
//		std::cout << YELLOW_PALE << "<" << fd << "> Disconnected" << RESET << std::endl; //Ya se hace en clearClient()
        clearClient(fd);
		//        close(fd); PUESTO DENTRO DE clearClient()
        return;
    }
	
    // Creamos puntero al obj cliente y guardamos los datos en su buffer
    Client* cli = getClient(fd);
    if (!cli)
		return;
    cli->appendToBuffer(std::string(buff, bytes));
	
	// Obtiene todo lo que hay en buffer del cliente
    std::string& buffer = cli->getBuffer();
    size_t pos;
	
    // Procesar solo cuando haya una línea completa
    while ((pos = buffer.find("\r\n")) != std::string::npos)
    {
		std::string command = buffer.substr(0, pos);  // comando completo
        buffer.erase(0, pos + 2);                     // elimina del buffer lo extraido
		
        if (!command.empty())
        {
			std::cout << "<" << fd << "> " << RED << "<< " << RESET << command << std::endl;
            parseCommand(cli, command); // <-- ahora siempre es una línea completa
        }
		//Nota de Sergio:
		//<--NO BORRAR CLIENTES DESDE LOS HANDLERS
		// O FALLARÁ AL VOLVER A ENTRAR AL WHILE Y LLAMAR A buffer.find
		// YA QUE buffer ES UNA REFERENCIA AL _buffer DEL CLIENTE BORRADO!!!!!!!!!!!
    }
}

void Server::sendToClient(Client& client, const std::string& message)
{
    if (client.isMarkedForRemoval()) return; // Si el cliente se ha de eliminar, No enviarle nada

    int fd = client.getFd();
    const char* buffer = message.c_str();
    size_t totalSent = 0;
    size_t length = message.size();

    while (totalSent < length)
    {
        ssize_t sent = send(fd, buffer + totalSent, length - totalSent, 0);
        if (sent <= 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) break; // El socket no está listo → esperar y reintentar
			clearClient(fd);
            return;
        }
        totalSent += sent;
    }
	// Muestra mensaje en consif (cli->getStatus() != AUTHENTICATED)ola sin \r\n
    std::cout << "<" << fd << "> " << GREEN << ">> " 
              << RESET << Utils::stripCRLF(message) << std::endl;
}




void Server::parseCommand(Client* cli, const std::string& cmd)
{
	if (cmd.empty())
		return ;

	if (cli->getStatus() != AUTHENTICATED) // || cli->getStatus() != NICK_AGAIN) //Si fallo el 1er Nick, maneja 2nd y 3rd
	{
		handshake(cli, cmd);
		return;
	}

	// if (cli->getStatus() != AUTHENTICATED && cli->getStatus() != USER_OK)
	// {
	// 	handshake(cli, cmd);
	// 	return;
	// }
		
	// // Si el cliente llega a USER_OK → lo marcamos como AUTHENTICATED
	// if (cli->getStatus() == USER_OK)
	// {
	// 	cli->setStatus(AUTHENTICATED);
	// 	//sendToClient(cli->getFd()
	// 	sendToClient(*cli, ":server 001 " + cli->getNickname() + " :Welcome to the IRC server!\r\n");
	// 	std::cout << YELLOW_PALE << "<" << cli->getFd() << "> " 
	// 	<< " Authenticated " << cli->getNickname() << RESET << std::endl;
	// }


	// Separar en tokens por espacio
	std::vector<std::string> tokens = Utils::split(cmd, ' ');
	if (tokens.empty())
		return ;

	std::string command = tokens[0];

	for (size_t i = 0; i < command.size(); ++i) {
		command[i] = std::toupper(command[i]);
		//command[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(command[i])));//uppercase seguro
	}

	//COMANDOS MINIMOS A MANEJAR IMPUESTOS POR SUBJECT Y HEXCHAT
	if (command == "CAP" || command == "CAP END") { // ignora estos comandos si vuelven de nuevo
		return ;
	}
	else if (command == "PASS") { // No se deberia poder cambiar
		handlePass(cli, tokens);  // si lo envia de nuevo: cerrar conexion
	}
	else if (command == "NICK") { // permitir cambio de nick
		handleNick(cli, tokens);
	}
	else if (command == "USER") {
		return;
	}
	else if (command == "JOIN")
	{
		handleJoin(cli, tokens);
	}
	else if (command == "WHO") { //Refresca datos, OJO podemos configurar HexChat para que no lo lance
		handleWho(cli, tokens);
	}
	else if (command == "KICK") {
		handleKick(cli, tokens);
	}
	else if (command == "PRIVMSG") {
		handlePrivmsg(cli, tokens);
	}
	else if (command == "TOPIC") {
		handleTopic(cli, tokens);
	}
	else if (command == "MODE") {
		handleMode(cli, tokens);
	}
	else if (command == "QUIT") { //gestionar una eliminacion y borrado limpios del cliente
	 	clearClient(cli->getFd()); // cierra socket,lo marca para eliminarlo de _fds, _clients, _channels y lo informa en consola 
	}
	else {
		std::cout << "Unknown command: " << cmd << std::endl;
		//Aqui llegaran los comandos que maneja HexChat y nosotros no hemos contemplado por que subject no lo requiere
		// quiza lo mejor sea no hacer nada aqui ?????
	}
}


// HANDSHAKE: autentica al cliente
void Server::handshake(Client *cli, const std::string &cmd)
{
	std::vector<std::string> tokens = Utils::split(cmd, ' ');
	if (tokens.empty())
		return;

	std::string command = tokens[0];

	//Convertir a mayúsculas por seguridad
	for (size_t i = 0; i < command.size(); ++i)
	{
		command[i] = std::toupper(command[i]);
		//command[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(command[i])));//uppercase seguro
	}

	if (command == "CAP" && cli->getStatus() == NOT_AUTHENTICATED)
	{
		handleCap(cli); //, tokens);
	}
	else if (command == "PASS" && cli->getStatus() == CAP_NEGOTIATED)
	{
		handlePass(cli, tokens);
	}
	else if (command == "NICK" && (cli->getStatus() == PASS_OK || cli->getStatus() == NICK_AGAIN))
	{
		handleNick(cli, tokens);
	}
	else if (command == "USER" && (cli->getStatus() == NICK_OK || cli->getStatus() == NICK_AGAIN))
	{
		handleUser(cli, tokens);
	}
	else if (command == "CAP END") //&& cli->getStatus() == USER_OK) //Ignoramos este comando
		return;
	else
	{
		//sendToClient(cli->getFd(), "451 :You have not registered\r\n");
		sendToClient(*cli, "451 :You have not registered\r\n");
	}

	// Si el cliente llega a USER_OK → lo marcamos como AUTHENTICATED
	if (cli->getStatus() == USER_OK)
	{
		cli->setStatus(AUTHENTICATED);
		//sendToClient(cli->getFd()
		std::cout << YELLOW_PALE << "<" << cli->getFd() << "> " 
					<< " Authenticated " << cli->getNickname() << RESET << std::endl;
		sendToClient(*cli, ":ircserv 001 " 	+ cli->getNickname() 
											+ " :Welcome to the IRC server " 
											+ cli->getNickname() + "!\r\n");

//		sendWelcomeMessages(*cli); //Desreferencio el puntero cli para pasarlo como referencia
	}
}
//DEBATIR CON SERGIO: esto no lo pide el subject y esta hardcodeado
/*
void Server::sendWelcomeMessages(Client& client)
{
    std::string nick = client.getNickname();

    // 001 RPL_WELCOME
    sendToClient(client, ":ircserv 001 " + nick + " :Welcome to the IRC server, " +
                            nick + "!" + client.getUsername() + "@localhost\r\n");

    // 002 RPL_YOURHOST
    sendToClient(client, ":ircserv 002 " + nick +
                            " :Your host is ircserv, running version 1.0.0\r\n");

    // 003 RPL_CREATED
    sendToClient(client, ":ircserv 003 " + nick +
                            " :This server was created Tue Sep 10 2025 at 12:00:00 UTC\r\n");

    // 004 RPL_MYINFO
    sendToClient(client, ":ircserv 004 " + nick +
                            " ircserv 1.0.0 \r\n");

    // 005 RPL_ISUPPORT (puedes añadir más líneas si quieres)
    sendToClient(client, ":ircserv 005 " + nick +
                            " CHANTYPES=# PREFIX=(ov)@+ NICKLEN=30 MAXCHANNELS=20 :are supported by this server\r\n");
}
*/


//RESPUESTA SOBRE LA LISTA DE CAPACIDADES QUE SOPORTA EL SERVER
void Server::handleCap(Client *cli) //, const std::vector<std::string> &tokens)
{
	// Respondemos con lista vacia de capabilities (:)
	std::ostringstream reply;
	std::string nick = cli->getNickname().empty() ? "nickname" : cli->getNickname();
	reply << ":Server CAP "
		  << nick
		  << " LS :\r\n";

	//sendToClient(cli->getFd(), reply.str());
	sendToClient(*cli, reply.str());
	cli->setStatus(CAP_NEGOTIATED);
}

void Server::handlePass(Client* cli, const std::vector<std::string>& tokens)
{
	if (!cli) {
		return;
	 }

	std::string target;
	if (cli->getNickname().empty())
		target = "*";
	else
		target = cli->getNickname();

	if (tokens.size() < 2) {
		sendToClient(*cli, ":ircserv 461 " + target + " PASS :Not enough parameters\r\n");
		//OPCIONAL: cerrar la conexion con el client aqui
		return ;
	}

	if (cli->getStatus() == AUTHENTICATED) { // Si ya estaba authenticado previamente
		sendToClient(*cli, ":ircserv 462 " + target + " :You may not reregister\r\n");
	 	return ;
	}

	const std::string& passArg = tokens[1]; // puede venir con ':' delante en algunos clientes OJO: DEBATIR con Sergio
	std::string pwd = passArg;

	//NO HARIA FALTA CON HEXCHAT: valorar con Sergio
	// Elimina ':' si preceden a la password
	if (!pwd.empty() && pwd[0] == ':')
		pwd.erase(0, 1);

	if (_password != pwd) {
		sendToClient(*cli, ":ircserv 464 " + target + " :Password incorrect\r\n");
//		cli->markForRemoval();
//		clearClient(cli->getFd());
		return;
	}

	//cli->setPassAccepted(true);
	cli->setStatus(PASS_OK);
//	sendToClient(*cli, "NOTICE AUTH :Password accepted\r\n");
	//tryRegister(*cli);
}

void Server::handleNick(Client* cli, const std::vector<std::string>& tokens)
{
	if (!cli)
		return ;
		
	std::string target;
    if (cli->getNickname().empty()) {
        target = "*";
	} else {
        target = cli->getNickname();
	}
	
	if (tokens.size() < 2) {
		sendToClient(*cli, ":ircserv 431 " + target + " :No nickname given\r\n");
		return ;
	}

	std::string newNick = tokens[1];
	
	// Validacion basica caracteres del NICK
	for (size_t i = 0; i < newNick.size(); ++i)
	{
		unsigned char ch = static_cast<unsigned char>(newNick[i]);

		if (i > 14 || (!std::isalnum(ch) && ch != '[' && ch != ']' && ch != '{' \
			&& ch != '}' && ch != '\\' && ch != '|' && ch != '_')) {
			sendToClient(*cli, ":ircserv 432 " + newNick + " :Erroneous nickname\r\n");
			//Si llega aqui setear flag como NICK_AGAIN, seguir a handleUser, 
			//guardar USER, y cuando vuelva a entrar Nick volvera a pasar por handleNick
			cli->setStatus(NICK_AGAIN);
			return;
		}
	}

	// Valida NICK que no este registrado previamente
	for (size_t i = 0; i < _clients.size(); ++i)
	{
		if (_clients[i].getFd() != cli->getFd() && _clients[i].getNickname() == newNick)
		{
			sendToClient(*cli, ":ircserv 433 " + newNick + " :Nickname is already in use\r\n");
			return ;
		}
	}

	//ALTERNATIVA: consultar con Sergio
	// if (getClientByNick(newNick))// 433: nick ya en uso
    // {
    //     sendToClient(*cli, ":ircserv 433 * " + newNick + " :Nickname is already in use\r\n");
    //     return ;
    // }

	cli->setNickname(newNick);
	sendToClient(*cli, ":* NICK " + newNick + "\r\n");
	//Alternativa: valorar con Sergio
	//sendToClient(*cli, ":ircserv NOTICE AUTH :Nickname set to " + newNick + "\r\n");
	if (cli->getStatus() == NICK_AGAIN) {
		cli->setStatus(USER_OK);
	} else {
		cli->setStatus(NICK_OK);
	}
//	tryRegister(*cli);
}

void Server::handleUser(Client* cli, const std::vector<std::string>& tokens)
{
	if (!cli)
		return ;

	std::string target;
    if (cli->getNickname().empty()) {
        target = "*";
	}
    else {
        target = cli->getNickname();
	}

	// std::string nick;
	// nick = cli->getNickname();

	// USER <username> <mode> <unused> :<realname with spaces>
	if (tokens.size() < 4)
	{
		sendToClient(*cli, ":ircserv 461 " + target + " USER :Not enough parameters\r\n");
		return ;
	}
	// if (cli->isAuthenticated())
	// {
	// 	sendToClient(*cli, "462 :You may not reregister\r\n");
	// 	return ;
	// }

	cli->setUsername(tokens[1]);
	
	// Realname puede empezar desde tokens[4] si existe, unido; si no hay, usa tokens[3]
	std::string realname;
	if (tokens.size() >= 5) {
		realname = Utils::joinFrom(tokens, 4, " ");
	} else {
		realname = tokens[3];
	}
	if (!realname.empty() && realname[0] == ':')
		realname.erase(0, 1);
	
	cli->setRealname(realname);

	if (cli->getStatus() == NICK_AGAIN)
		return;
	else
		cli->setStatus(USER_OK);

//	tryRegister(*cli);
}


void Server::handleJoin(Client* cli, const std::vector<std::string>& tokens)
{
	// std::string nick;
	// nick = cli->getNickname();

    if (tokens.size() < 2)//if (tokens.empty())
    {
        sendToClient(*cli, ":ircserv 461 JOIN :Not enough parameters\r\n");
        return ;
    }

    std::string channelName = tokens[1];
    std::string keyArg; // clave opcional
    if (tokens.size() >= 3)
        keyArg = tokens[2]; // el cliente puede enviar la key

    if (channelName[0] != '#')/////Añadir restricciones!!!!!! (caracteres, longitud, etc)
    {
        sendToClient(*cli, ":ircserv 479 " + channelName + " :Illegal channel name\r\n");
        return ;
    }

    Channel* chan = getOrCreateChannel(channelName);

    // 1. Comprobar invite-only (+i)
    if (chan->isModeI() && !chan->isOperator(cli->getFd()))
    {
        sendToClient(*cli, ":ircserv 473 " + channelName + " :Cannot join channel (+i)\r\n");
        return ;
    }

    // 2. Comprobar clave (+k)
    if (chan->isModeK())
    {
        if (keyArg.empty() || keyArg != chan->getKey())
        {
            sendToClient(*cli, "ircserv 475 " + channelName + " :Cannot join channel (+k)\r\n");
            return ;
        }
    }

    // 3. Comprobar límite de usuarios (+l)
    if (chan->isModeL() && static_cast<int>(chan->getClients().size()) >= chan->getUserLimit())
    {
        sendToClient(*cli, "ircserv 471 " + channelName + " :Cannot join channel (+l)\r\n");
        return ;
    }

    // 4. Primer usuario → operador
    bool isFirst = chan->getClients().empty();
    chan->addClient(cli->getFd(), isFirst);

    // Avisar JOIN al propio cliente y a los demas
    std::string joinMsg = ":" + cli->getNickname() + "!" + cli->getUsername() +
                          "@" + cli->getIp() + " JOIN " + channelName + "\r\n";

    sendToClient(*cli, joinMsg);
    const std::set<int>& members = chan->getClients();
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        if (*it != cli->getFd())
        {
            Client *other = getClient(*it);
            if (other)
                sendToClient(*other, joinMsg);
        }
    }

    // (Opcional) Topic actual ---
    if (chan->hasTopic())
    {
        sendToClient(*cli, ":ircserv 332 " + cli->getNickname() + " " + channelName +
                              " :" + chan->getTopic() + "\r\n");
    }
    sendToClient(*cli, ":ircserv 333 " + cli->getNickname() + " " + channelName +
                          " " + cli->getNickname() + " 0\r\n");

    // Lista de nicks inicial(RPL_NAMREPLY + RPL_ENDOFNAMES)
    std::ostringstream names;
    names << ":ircserv 353 " << cli->getNickname() << " = " << channelName << " :";

    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        Client *m = getClient(*it);
        if (!m) continue;
        if (chan->isOperator(*it))
            names << "@" << m->getNickname() << " ";
        else
            names << m->getNickname() << " ";
    }
    names << "\r\n";
    sendToClient(*cli, names.str());

    sendToClient(*cli, ":ircserv 366 " + cli->getNickname() + " " + channelName +
                          " :End of NAMES list\r\n");
}


// Busca un canal por nombre en _channels
Channel* Server::getChannelByName(const std::string &name)
{
    for (size_t i = 0; i < _channels.size(); ++i)
    {
        if (_channels[i].getName() == name)
            return &_channels[i];
    }
    return NULL;
}
//Periodicamente HexChat lanza un WHO para actualizar lista de clientes del canal
//Se puede configurar HexChat para que NO lo lance y muestre solo los que habia al entrar
void Server::handleWho(Client *cli, const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
        return; // WHO sin argumento

    const std::string &channelName = tokens[1];
    Channel *chan = getChannelByName(channelName);

    // Si no hay canal, enviar solo fin de WHO vacio
    if (!chan)
    {
        std::ostringstream end;
        end << ":" << "ircserv"
            << " 315 " << cli->getNickname()
            << " " << channelName << " :End of WHO list\r\n";
        sendToClient(*cli, end.str());
        return;
    }

    // Recorremos los fd de los miembros del canal
    const std::set<int> &members = chan->getClients();
    for (std::set<int>::const_iterator it = members.begin();
         it != members.end(); ++it)
    {
        int memberFd = *it;
        Client *member = getClient(memberFd);
        if (!member)
            continue;

        // Flags: H (here) y @ si es operador
        std::string flags = "H";
        if (chan->isOperator(memberFd))
            flags += "@";

        std::ostringstream reply;
        reply << ":" << "ircserv"
              << " 352 " << cli->getNickname()    // el que envio el WHO
              << " " << channelName               // canal
              << " " << member->getUsername()     // <usuario>
              << " " << member->getIp()           // <host>
              << " " << "ircserv"      // servidor
              << " " << member->getNickname()     // <nick>
              << " " << flags                     // <flags>
              << " :0 " << member->getRealname()  // hopcount y realname
              << "\r\n"; //OJO creo que // ya lo anyadimos en sendToClient()

        sendToClient(*cli, reply.str());
    }

    // Fin de WHO list
    std::ostringstream end;
    end << ":" << "ircserv"
        << " 315 " << cli->getNickname()
        << " " << channelName << " :End of WHO list\r\n"; //OJO creo que // ya lo anyadimos en sendToClient()
    sendToClient(*cli, end.str());
}





void Server::handlePrivmsg(Client *cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;
    
    std::string target;
    if (cli->getNickname().empty())
        target = "*";
    else
        target = cli->getNickname();

    // if (!cli->isAuthenticated())
    // {
    //     sendToClient(*cli, ":ircserv 451 " + target + " :You have not registered\r\n");
    //     return ;
    // }
    
    // 1. Validar parámetros
    if (tokens.size() < 3)
    {
        sendToClient(*cli, ":ircserv 461 " + cli->getNickname() + " PRIVMSG :Not enough parameters\r\n");
        return ;
    }

    //std::string
    target = tokens[1];//tokens[0];  // puede ser nick o canal
    std::string message = tokens[2];

    // El mensaje puede empezar con ':' y tener espacios
    if (tokens[2][0] == ':')
        message = tokens[2].substr(1);              // quitar el ':'
    for (size_t i = 3; i < tokens.size(); ++i)      // Si luego de message hay más tokens -> unificarlos en un message
        message += " " + tokens[i];
        
    // 2. Decidir si es un channel o un client
    if (target[0] == '#') // => es un channel (quitar el '#' NO HACE FALTA PORQUE EL NOMBRE SE GUARDA CON #)
    {
        Channel *chan = NULL;//nullptr;
        for (size_t i = 0; i < _channels.size(); i++)
        {
            if (_channels[i].getName() == target)
            {
                chan = &_channels[i];
                break ;
            }
        }

        if (!chan)
        {
            sendToClient(*cli, ":ircserv 403 " + cli->getNickname() + " " + target + " :No such channel\r\n");
            return ;
        }

        if (!chan->isMember(cli->getFd()))//if (!chan.hasClient(cli->getFd()))
        {
            sendToClient(*cli, ":ircserv 442 " + cli->getNickname() + " " + target + " :You're not on that channel\r\n");
            return ;
        }
       
        // reenviar a todos los clientes del canal excepto el emisor
        const std::set<int>& members = chan->getClients();
        for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
        {
            if (*it != cli->getFd())
            {
                Client* other = getClient(*it);
                if (other)
                {
                    //sendToClient(*other, ":" + cli->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n");
                    sendToClient(*other, ":" + cli->getNickname() + "!" + cli->getUsername() + "@localhost" + " PRIVMSG " + target + " :" + message + "\r\n");
                }
            }
        }
    }
    else
    {
        // --- PRIVMSG a usuario ---
        Client* targetClient = getClientByNick(target);
        if (!targetClient)
        {
            sendToClient(*cli, ":ircserv 401 " + cli->getNickname() + " " + target + " :No such nick\r\n");
            return ;
        }
        //sendToClient(*targetClient, ":" + cli->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n");
        sendToClient(*targetClient, ":" + cli->getNickname() + "!" + cli->getUsername() + "@localhost" + " PRIVMSG " + target + " :" + message + "\r\n");
    }
}

// void Server::handleQuit(Client* cli) {
// 	clearClient(pasarle el fd);
// 	// marcamos para borrar
// }


Client* Server::getClientByNick(const std::string& nick)
{
    for (size_t i = 0; i < _clients.size(); ++i)
    {
        if (_clients[i].getNickname() == nick)
        {
            return &_clients[i];
        }
    }
    return NULL; // no encontrado
}


Channel* Server::getOrCreateChannel(const std::string& name)
{
    for (size_t i = 0; i < _channels.size(); i++)
    {
        if (_channels[i].getName() == name)
            return &_channels[i];
    }
    _channels.push_back(Channel(name));
    return &_channels.back();
}

Channel* Server::findChannel(const std::string& channelName)
{
    Channel* chan = NULL;

    for (size_t i = 0; i < _channels.size(); ++i)
    {
        if (_channels[i].getName() == channelName)
        {
            chan = &_channels[i];
            break ;
        }
    }
    return (chan);
}

void Server::handleTopic(Client *cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;
    
    std::string target;
    if (cli->getNickname().empty()) {
        target = "*";
	}
    else {
        target = cli->getNickname();
	}

//    if (!cli->isAuthenticated())
	if (cli->getStatus() != AUTHENTICATED)
    {
        sendToClient(*cli, ":ircserv 451 " + target + " :You have not registered\r\n");
        return ;
    }
    
    if (tokens.size() < 2)
    {
        sendToClient(*cli, ":ircserv 461 " + cli->getNickname() + " TOPIC :Not enough parameters\r\n");
        return ;
    }

    std::string channelName = tokens[1];

    // Buscar canal en vector
    Channel* chan = NULL;
    for (size_t i = 0; i < _channels.size(); ++i)
    {
        if (_channels[i].getName() == channelName)
        {
            chan = &_channels[i];
            break ;
        }
    }

    if (!chan)
    {
        sendToClient(*cli, ":ircserv 403 " + cli->getNickname() + " " + channelName + " :No such channel\r\n");
        return ;
    }

    if (!chan->isMember(cli->getFd()))
    {
        sendToClient(*cli, ":ircserv 442 " + cli->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return ;
    }

    // Caso 1: Solo consultar topic
    if (tokens.size() == 2)
    {
        if (chan->hasTopic())
            sendToClient(*cli, ":ircserv 332 " + cli->getNickname() + " " + channelName + " :" + chan->getTopic() + "\r\n");
        else
            sendToClient(*cli, ":ircserv 331 " + cli->getNickname() + " " + channelName + " :No topic is set\r\n");
        return ;
    }
    // Caso 2: Cambiar topic (Si modo +t está activo, solo operadores pueden)
    if (chan->isModeT() && !chan->isOperator(cli->getFd()))
    {
        sendToClient(*cli, ":ircserv 482 " + cli->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return ;
    }

    // Reconstruir el nuevo topic
    std::string topic;
    if (tokens[2][0] == ':')
        topic = tokens[2].substr(1);
    else
        topic = tokens[2];
    for (size_t i = 3; i < tokens.size(); ++i)
        topic += " " + tokens[i];

    chan->setTopic(topic);

    // Avisar a todos los miembros del canal
    const std::set<int>& members = chan->getClients();
    for (std::set<int>::const_iterator mit = members.begin(); mit != members.end(); ++mit)
    {
        Client* other = getClient(*mit);
        if (other)
            sendToClient(*other, ":" + cli->getNickname() + " TOPIC " + channelName + " :" + topic + "\r\n");
    }
}

void Server::handleMode(Client* cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;
    
    std::string target;
    if (cli->getNickname().empty()) {
        target = "*";
	}
    else {
        target = cli->getNickname();
	}

    //if (!cli->isAuthenticated())
	if (cli->getStatus() != AUTHENTICATED)
    {
        sendToClient(*cli, ":ircserv 451 " + target + " :You have not registered\r\n");
        return ;
    }
    
    if (tokens.size() < 2)
    {
        sendToClient(*cli, ":ircserv 461 " + target + " MODE :Not enough parameters\r\n");
        return ;
    }

    target = tokens[1];
    if (target[0] != '#')// Comprobar que channel comienza por '#'
    {
        sendToClient(*cli, ":ircserv 403 " + target + " :No such channel\r\n");
        return ;
    }

    // Buscar canal en el vector
    Channel* chan = NULL;
    for (size_t i = 0; i < _channels.size(); ++i)
    {
        if (_channels[i].getName() == target)
        {
            chan = &_channels[i];
            break ;
        }
    }

    if (!chan)
    {
        sendToClient(*cli, ":ircserv 403 " + target + " :No such channel\r\n");
        return ;
    }

// Caso 1: consulta modos
    if (tokens.size() == 2) {
        showChannelModes(cli, chan, target);
        return;
    }

    // Caso 2: modificación de modos
    if (!checkOperator(cli, chan, target))
        return;

    applyChannelModes(cli, chan, tokens, target);

/*  Caso 1: Si no hay flags, mostrar modos actuales
    if (tokens.size() == 2)
    {
        std::string modes = "";
        if (chan->isModeT()) modes += "t";
        if (chan->isModeI()) modes += "i";
        if (chan->isModeK()) modes += "k";
        if (chan->isModeL()) modes += "l";

        if (!modes.empty())
            sendToClient(*cli, ":ircserv 324 " + cli->getNickname() + " " + target + " +" + modes + "\r\n");
        else
            sendToClient(*cli, ":ircserv 324 " + cli->getNickname() + " " + target + "\r\n");
        return ;
    }

    // Caso 2: Validar que sea operador para modificar los modes
    if (!chan->isOperator(cli->getFd()))
    {
        sendToClient(*cli, ":ircserv 482 " + cli->getNickname() + " " + target + " :You're not channel operator\r\n");
        return ;
    }
    // Procesar flags
    std::string modeStr = tokens[2];
    bool add = true;
    size_t argIndex = 3; // índice para argumentos opcionales (key o limit)

    for (size_t i = 0; i < modeStr.size(); ++i)
    {
        char m = modeStr[i];
        if (m == '+') { add = true; continue; }
        if (m == '-') { add = false; continue; }

        if (m == 't')
            chan->setModeT(add);
        else if (m == 'i')
            chan->setModeI(add);
        else if (m == 'k')
        {
            if (add)
            {
                if (argIndex >= tokens.size())
                {
                    sendToClient(*cli, "461 MODE :Not enough parameters for +k\r\n");
                    continue;
                }
                chan->setKey(tokens[argIndex++]);
            }
            else
            {
                chan->setModeK(false);
                chan->setKey("");
            }
        }
        else if (m == 'l')
        {
            if (add)
            {
                if (argIndex >= tokens.size())
                {
                    sendToClient(*cli, "461 MODE :Not enough parameters for +l\r\n");
                    continue;
                }
                int limit = std::atoi(tokens[argIndex++].c_str());
                if (limit <= 0)
                {
                    sendToClient(*cli, "461 MODE :Invalid user limit\r\n");
                    continue;
                }
                chan->setUserLimit(limit);
            }
            else
            {
                chan->setModeL(false);
                chan->setUserLimit(0);
            }
        }
        else
        {
            sendToClient(*cli, ":ircserv 472 " + cli->getNickname() + std::string(1, m) + " :is unknown mode char\r\n");
        }
    }

    // Notificar a todos en el canal
    const std::set<int>& members = chan->getClients();
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        Client* other = getClient(*it);
        if (other)
        {
            sendToClient(*other, ":" + cli->getNickname() + " MODE " + target + " " + modeStr + "\r\n");
        }
    }*/
}

// --- Funciones auxiliares ---

bool Server::checkOperator(Client *cli, Channel *chan, const std::string& target)
{
    if (!chan->isOperator(cli->getFd()))
    {
        sendToClient(*cli, ":ircserv 482 " + cli->getNickname() + " " + target + " :You're not channel operator\r\n");
        return false;
    }
    return true;
}

void Server::showChannelModes(Client *cli, Channel *chan, const std::string& target)
{
    std::string modes = "";
    if (chan->isModeT()) modes += "t";
    if (chan->isModeI()) modes += "i";
    if (chan->isModeK()) modes += "k";
    if (chan->isModeL()) modes += "l";

    if (!modes.empty())
        sendToClient(*cli, ":ircserv 324 " + cli->getNickname() + " " + target + " +" + modes + "\r\n");
    else
        sendToClient(*cli, ":ircserv 324 " + cli->getNickname() + " " + target + "\r\n");
}

void Server::broadcastModeChange(Channel* chan, Client* cli,
                                const std::string& target,
                                const std::string& appliedModes,
                                const std::vector<std::string>& appliedArgs) 
{
    if (appliedModes.empty())
        return ;

    std::string notice = ":" + cli->getNickname() + " MODE " + target + " " + appliedModes;
    for (size_t i = 0; i < appliedArgs.size(); ++i)
        notice += " " + appliedArgs[i];
    notice += "\r\n";

    const std::set<int>& members = chan->getClients();
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        Client* other = getClient(*it);
        if (other)
            sendToClient(*other, notice);
    }
}

void Server::applyChannelModes(Client *cli, Channel *chan,
                               const std::vector<std::string>& tokens,
                               const std::string& target) 
{
    std::string modeStr = tokens[2];
    bool add = true;
    size_t argIndex = 3;

    std::string appliedModes = "";
    std::vector<std::string> appliedArgs;

    for (size_t i = 0; i < modeStr.size(); ++i)
    {
        char m = modeStr[i];
        if (m == '+') { add = true; appliedModes += '+'; continue; }
        if (m == '-') { add = false; appliedModes += '-'; continue; }

        if (m == 't')
        {
            chan->setModeT(add);
            appliedModes += 't';
        }
        else if (m == 'i')
        {
            chan->setModeI(add);
            appliedModes += 'i';
        }
        else if (m == 'k')
        {
            if (add)
            {
                if (argIndex >= tokens.size())
                {
                    sendToClient(*cli, ":ircserv 461 " + cli->getNickname() + " MODE :Not enough parameters for +k\r\n");
                    continue;
                }
                chan->setKey(tokens[argIndex]);
                chan->setModeK(true);
                appliedModes += 'k';
                appliedArgs.push_back(tokens[argIndex++]);
            }
            else
            {
                chan->setModeK(false);
                chan->setKey(""); // limpiar
                appliedModes += 'k';
            }
        }
        else if (m == 'l')
        {
            if (add)
            {
                if (argIndex >= tokens.size())
                {
                    sendToClient(*cli, ":ircserv 461 " + cli->getNickname() + " MODE :Not enough parameters for +l\r\n");
                    continue;
                }
                char* endptr = NULL;
                long limit = strtol(tokens[argIndex].c_str(), &endptr, 10);
                if (*endptr != '\0' || limit <= 0)
                {
                    sendToClient(*cli, ":ircserv 461 " + cli->getNickname() + " MODE :Invalid user limit\r\n");
                    argIndex++;
                    continue;
                }
                chan->setUserLimit((int)limit);
                chan->setModeL(true);
                appliedModes += 'l';
                appliedArgs.push_back(tokens[argIndex++]);
            }
            else
            {
                chan->setModeL(false);
                chan->setUserLimit(0);
                appliedModes += 'l';
            }
        }
        else if (m == 'o')
        {
            if (argIndex >= tokens.size())
            {
                sendToClient(*cli, ":ircserv 461 " + cli->getNickname() + " MODE :Not enough parameters for +o/-o\r\n");
                continue;
            }
            std::string nick = tokens[argIndex++];
            Client* targetCli = getClientByNick(nick);
            if (!targetCli || !chan->isMember(targetCli->getFd()))
            {
                sendToClient(*cli, ":ircserv 441 " + nick + " " + target + " :They aren't on that channel\r\n");
                continue;
            }

            if (add)
                chan->addOperator(targetCli->getFd());
            else
                chan->removeOperator(targetCli->getFd());

            appliedModes += 'o';
            appliedArgs.push_back(nick);
        }
        else
        {
            sendToClient(*cli, ":ircserv 472 " + cli->getNickname() + " " 
                               + std::string(1, m) + " :is unknown mode char\r\n");
        }
    }

    broadcastModeChange(chan, cli, target, appliedModes, appliedArgs);
}

void Server::handleKick(Client *cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;
    
    std::string target;
    if (cli->getNickname().empty()) {
        target = "*";
	}
    else {
        target = cli->getNickname();
	}

    //if (!cli->isAuthenticated())
	if (cli->getStatus() != AUTHENTICATED)
    {
        sendToClient(*cli, ":ircserv 451 " + target + " :You have not registered\r\n");
        return ;
    }
    
    if (tokens.size() < 3)
    {
        sendToClient(*cli, ":ircserv 461 " + cli->getNickname() + " KICK :Not enough parameters\r\n");
        return ;
    }

    std::string channelName = tokens[1];
    std::string targetNick  = tokens[2];

    Channel* chan = findChannel(channelName);
    
    if (!chan)
    {
        sendToClient(*cli, ":ircserv 403 " + channelName + " :No such channel\r\n");
        return ;
    }

    if (!chan->isOperator(cli->getFd()))
    {
        sendToClient(*cli, ":ircserv 482 " + channelName + " :You're not channel operator\r\n");
        return ;
    }

    Client* targetCli = getClientByNick(targetNick);
    if (!targetCli || !chan->isMember(targetCli->getFd()))
    {
        sendToClient(*cli, ":ircserv 441 " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
        return ;
    }

    // Mensaje de razón (opcional, después del nick)
    //std::string reason = (tokens.size() > 3) ? tokens[3] : cli->getNickname();
    std::string reason;
    if (tokens.size() > 3)
        reason = tokens[3];
    else
        reason = cli->getNickname();

    // Avisar a todos en el canal
    const std::set<int>& members = chan->getClients();
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        Client* other = getClient(*it);
        if (other)
        {
            sendToClient(*other, ":" + cli->getNickname() + " KICK " + channelName +
                                  " " + targetNick + " :" + reason + "\r\n");
        }
    }

    // Expulsar al cliente del canal
    chan->removeClient(targetCli->getFd());

	//Si el canal ha quedado vacio, porque el unico
	// miembro era el operator y se expulso a si
	//mismo con el KICK, debemos eliminar el canal.
	if (chan->getClients().empty()) {
    	removeEmptyChannel(chan); // es un puntero al canal
	}
}

void Server::removeEmptyChannel(Channel* chan) { 
	if (!chan)
		return;
	for (std::vector<Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (&(*it) == chan) {
			_channels.erase(it);
			break;
		}
	}
}



void Server::handleInvite(Client* cli, const std::vector<std::string>& tokens)
{
    if (tokens.size() < 3)
    {
        sendToClient(*cli, ":ircserv 461 " + cli->getNickname() + " INVITE :Not enough parameters\r\n");
        return;
    }

    std::string targetNick  = tokens[1];
    std::string channelName = tokens[2];

    Channel* chan = findChannel(channelName);
    /*Channel *chan = NULL;
    for (size_t i = 0; i < _channels.size(); i++)
    {
        if (_channels[i].getName() == channelName)
        {
            chan = &_channels[i];
            std::cout << "Channel "<< _channels[i].getName() <<" found!" << std::endl;
            break ;
        }
    }*/
    if (!chan)
    {
        sendToClient(*cli, ":ircserv 403 " + channelName + " :No such channel\r\n");
        return;
    }

    Client* targetCli = getClientByNick(targetNick);
    if (!targetCli)
    {
        sendToClient(*cli, ":ircserv 401 " + targetNick + " :No such nick\r\n");
        return;
    }

    if (chan->isMember(targetCli->getFd()))
    {
        sendToClient(*cli, ":ircserv 443 " + targetNick + " " + channelName + " :is already on channel\r\n");
        return;
    }

    if (chan->isModeI() && !chan->isOperator(cli->getFd()))
    {
        sendToClient(*cli, ":ircserv 482 " + channelName + " :You're not channel operator\r\n");
        return;
    }

    chan->inviteClient(targetCli->getFd());

    // Avisar al invitado
    sendToClient(*targetCli, ":" + cli->getNickname() + " INVITE " + targetNick + " :" + channelName + "\r\n");

    // Confirmar al emisor
    sendToClient(*cli, ":ircserv 341 " + cli->getNickname() + " " + targetNick + " " + channelName + "\r\n");

    // Aquí podrías añadir lógica para marcar al cliente como "invitado" al canal
    // por ejemplo, un set<int> _invited en Channel para comprobarlo en handleJoin().
}
