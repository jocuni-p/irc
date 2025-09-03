#include "../include/Server.hpp"


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
// CREA Y PONE SERVER EN ESCUCHA
void Server::serverInit(int port, std::string& pwd)
{
	this->_port = port;
	this->_password = pwd;
	createSocket();
	
	std::cout << "** Server IRC created **" << std::endl;
	std::cout << "Listening on port <" << this->_port << ">" << std::endl;

	while (Server::_signalFlag == false)
	{
		if ((poll(&_fds[0], _fds.size(), -1) == -1) && Server::_signalFlag == false)
			throw(std::runtime_error("poll() failed"));

		// Recorre hacia atrás para poder borrar sin saltarte elementos
		for (int i = static_cast<int>(_fds.size()) - 1; i >= 0; --i)
		{
			if (!(_fds[i].revents & POLLIN))
				continue;
			
			if (_fds[i].fd == _serverFd)
				acceptNewClient();
			else
				receiveNewData(_fds[i].fd);
		}
	}
	closeFds(); 
}

// Elimina un cliente de _fds y de _clients
void Server::clearClient(int fd)
{
    // Cierra primero, ignora errores
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR); // mejor esfuerzo; ignora error si ya está cerrado
        close(fd);
    }

    // Lo elimina de _fds
    for (size_t i = 0; i < _fds.size(); ++i) {
        if (_fds[i].fd == fd) {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }
    // Lo elimina de _clients
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].getFd() == fd) {
            _clients.erase(_clients.begin() + i);
            break;
        }
    }
	std::cout << "<" << fd << "> Disconnected" << std::endl;
}


//Cierre total de estructuras y sockets limpiamente
void Server::closeFds()
{
    // Cierra clientes (forma limpia)
    for (size_t i = 0; i < _clients.size(); ++i) {
        int cfd = _clients[i].getFd();
        if (cfd >= 0) {
            std::cout << "<" << cfd << "> Disconnected" << std::endl;
            shutdown(cfd, SHUT_RDWR); // aviso y cierre de socket, forma mas limpia
            close(cfd); //libera el fd
        }
    }

    // Cierra el socket del servidor
    if (_serverFd != -1) {
        std::cout << "Server <" << _serverFd << "> Disconnected" << std::endl;
        shutdown(_serverFd, SHUT_RDWR);
        close(_serverFd);
        _serverFd = -1;
    }

    // Limpia estructuras (C++98 trick para liberar capacidad)
    std::vector<Client>().swap(_clients);
    std::vector<struct pollfd>().swap(_fds);

    std::cout << "** IRC Server shutdown gracefully **" << std::endl;
}



void Server::acceptNewClient()
{
    struct sockaddr_in cliAdd;
    socklen_t len = sizeof(cliAdd);
    int newFd = accept(_serverFd, (sockaddr *)&cliAdd, &len);
    if (newFd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return; // nada que aceptar
		std::perror("(!) ERROR:  accept()");
        return; // no continúes con fd inválido
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

	std::cout << "<" << newFd << "> Connected" << std::endl;
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
		std::cout << "<" << fd << "> Disconnected" << std::endl;
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
    }
}
// MOVIDO A UTILS
// // Helper para dividir un string en tokens
// std::vector<std::string> split(const std::string& str, char delim)
// {
//     std::vector<std::string> tokens;
//     std::stringstream ss(str);
//     std::string token;
//     while (std::getline(ss, token, delim))
//         tokens.push_back(token);
//     return tokens;
// }

static std::string joinFrom(const std::vector<std::string>& v, size_t start, const std::string& sep = " ")
{
    std::string out;
    for (size_t i = start; i < v.size(); ++i)
    {
        if (i > start)
            out += sep;
        out += v[i];
    }
    return out;
}


void Server::parseCommand(Client* cli, const std::string& cmd)
{
    if (cmd.empty())
        return ;

    // Separar en tokens por espacio
    std::vector<std::string> tokens = Utils::split(cmd, ' ');
    if (tokens.empty())
        return ;

    std::string command = tokens[0];

    // Convertir a mayúsculas por seguridad
    for (size_t i = 0; i < command.size(); ++i)
    {
        command[i] = std::toupper(command[i]);
        //command[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(command[i])));//uppercase seguro

    }

    // Enrutamos según comando
    if (command == "CAP" || command == "QUIT") {
        return ; //ignora estos comandos
    }
    if (command == "PASS") {
        handlePass(cli, tokens);
    }
    else if (command == "NICK") {
        handleNick(cli, tokens);
    }
    else if (command == "USER") {
        handleUser(cli, tokens);
    }
    else if (command == "JOIN") {
        handleJoin(cli, tokens);
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
    else {
        std::cout << "Unknown command: " << cmd << std::endl;
		//Aqui llegaran los comandos que maneja HexChat y nosotros no hemos contemplado por que subject no lo requiere
        // quiza lo mejor sea no hacer nada mas aqui
    }
}


void Server::handlePass(Client* cli, const std::vector<std::string>& tokens)
{
    if (!cli) {
		return;
	 }

	//if (tokens.size() < 2)
	if (tokens.size() != 2)
    {
        sendToClient(*cli, "461 PASS :Not valid number of parameters\r\n");
		//El server debe cerrar la conexion con el client aqui
        return ;
    }
    if (cli->isAuthenticated())
    {
        sendToClient(*cli, "462 :You may not reregister\r\n");
        return ;
    }

    const std::string& passArg = tokens[1]; // puede venir con ':' delante en algunos clientes OJO: DEBATIR con Sergio
    std::string pwd = passArg;

    if (!pwd.empty() && pwd[0] == ':')
        pwd.erase(0, 1);

    if (_password != pwd) {
        sendToClient(*cli, "464 :Password incorrect\r\n");
		clearClient(cli->getFd());
		return;
	}

    cli->setPassAccepted(true);
    sendToClient(*cli, "NOTICE AUTH :Password accepted\r\n");
    tryRegister(*cli);
}

void Server::handleNick(Client* cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;

    if (tokens.size() < 2) {
        sendToClient(*cli, "431 :No nickname given\r\n");
        return ;
    }

    std::string newNick = tokens[1];

////// Validación básica: alfanumérico (puedes ampliar a RFC si quieres)
    for (size_t i = 0; i < newNick.size(); ++i)
    {
        if (!std::isalnum(static_cast<unsigned char>(newNick[i])))
        {
            sendToClient(*cli, "432 " + newNick + " :Erroneous nickname\r\n");
            return ;
        }
    }

    // Unicidad
    for (size_t i = 0; i < _clients.size(); ++i)
    {
        if (_clients[i].getFd() != cli->getFd() && _clients[i].getNickname() == newNick)
        {
            sendToClient(*cli, "433 " + newNick + " :Nickname is already in use\r\n");
            return ;
        }
    }

    cli->setNickname(newNick);
    sendToClient(*cli, "NICK " + newNick + "\r\n");
    tryRegister(*cli);
}

void Server::handleUser(Client* cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;

    // USER <username> <mode> <unused> :<realname with spaces>
    if (tokens.size() < 4)
    {
        sendToClient(*cli, "461 USER :Not enough parameters\r\n");
        return ;
    }
    if (cli->isAuthenticated())
    {
        sendToClient(*cli, "462 :You may not reregister\r\n");
        return ;
    }

    cli->setUsername(tokens[1]);

    // Realname puede empezar desde tokens[4] si existe, unido; si no hay, usa tokens[3]
    std::string realname;
    if (tokens.size() >= 5) {
        realname = joinFrom(tokens, 4, " ");
    } else {
        realname = tokens[3];
    }
    if (!realname.empty() && realname[0] == ':')
        realname.erase(0, 1);

    cli->setRealname(realname);

    tryRegister(*cli);
}

//OJO solo para pruebas de joan
void Server::handleJoin(Client* cli, const std::vector<std::string>& tokens)
{
    (void)cli;
    std::cout << tokens[0] << " command received" << std::endl;
}
/*
void Server::handleJoin(Client* cli, const std::vector<std::string>& tokens)
{
    if (tokens.size() < 2)//if (tokens.empty())
    {
        sendToClient(*cli, "461 JOIN :Not enough parameters\r\n");
        return ;
    }

    std::string channelName = tokens[1];
    if (channelName[0] != '#')
    {
        sendToClient(*cli, "479 " + channelName + " :Illegal channel name\r\n");
        return ;
    }

    Channel *chan = getOrCreateChannel(channelName);

    // Primer usuario → operador
    bool isFirst = chan->getClients().empty();
    chan->addClient(cli->getFd(), isFirst);

    // Aviso al propio cliente
    sendToClient(*cli, ":" + cli->getNickname() + " JOIN " + channelName + "\r\n");

    // Aviso a los demás en el canal
    const std::set<int>& members = chan->getClients();
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        if (*it != cli->getFd())
        {
            Client* other = getClient(*it);
            if (other)
                sendToClient(*other, ":" + cli->getNickname() + " JOIN " + channelName + "\r\n");
        }
    }
}*/
	
//Temporal solo para pruebas de joan
void Server::handlePrivmsg(Client* cli, const std::vector<std::string>& tokens)
{
    (void)cli;
    std::cout << tokens[0] << " command received" << std::endl;
}
/*
void Server::handlePrivmsg(Client* cli, const std::vector<std::string>& tokens)
{
    // 1. Validar parámetros
    if (tokens.size() < 3)
    {
        sendToClient(*cli, ":localhost 461 " + cli->getNickname() + " PRIVMSG :Not enough parameters\r\n");
        return ;
    }

    std::string target = tokens[1];//tokens[0];  // puede ser nick o canal
    std::string message = tokens[2];

    // El mensaje puede empezar con ':' y tener espacios
    if (tokens[2][0] == ':')
        message = tokens[2].substr(1);              // quitar el ':'
    for (size_t i = 3; i < tokens.size(); ++i)      // Si luego de message hay más tokens -> unificarlos en un message
        message += " " + tokens[i];
        
    // 2. Decidir si es un channel o un client
    if (target[0] == '#') // => es un channel
    {
        //target = tokens[1].substr(1);              // quitar el '#' NO HACE FALTA PORQUE EL NOMBRE SE GUARDA CON #
        Channel *chan = 0;//nullptr;
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
            sendToClient(*cli, ":localhost 403 " + cli->getNickname() + " " + target + " :No such channel\r\n");
            return ;
        }

        if (!chan->isMember(cli->getFd()))//if (!chan.hasClient(cli->getFd()))
        {
            sendToClient(*cli, ":localhost 442 " + cli->getNickname() + " " + target + " :You're not on that channel\r\n");
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
                    sendToClient(*other, ":" + cli->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n");
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
            sendToClient(*cli, ":localhost 401 " + cli->getNickname() + " " + target + " :No such nick\r\n");
            return ;
        }
        sendToClient(*targetClient, ":" + cli->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n");
    }
}
*/

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

void Server::tryRegister(Client& client)
{
    if (client.isAuthenticated())
        return ;

    if (client.hasPassAccepted() && !client.getNickname().empty() && !client.getUsername().empty())
    {
        client.setAuthenticated(true);

        sendToClient(client, ":ircserv 001 " + client.getNickname() + " :Welcome to the IRC server, " 
                    + client.getNickname() + "!" + client.getUsername() + "@localhost\r\n");

//        std::cout << "<" << client.getFd() << "> authenticated as "
//                 << client.getNickname() << std::endl; // DEBUG
    }
}

void Server::sendToClient(Client& client, const std::string& message)
{
    int fd = client.getFd();
    const char* buffer = message.c_str();
    size_t totalSent = 0;
    size_t length = message.size();

    while (totalSent < length)
    {
        ssize_t sent = send(fd, buffer + totalSent, length - totalSent, 0);
		if (sent <= 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN) // El socket no está listo → esperar y reintentar
			break ;
            else
            {
				std::cerr << "Error sending to client <" << fd << ">" << std::endl;
                clearClient(fd); // ya tiene un close()
				// close(fd); no va porque es un doble close()
                return ;
            }
        }
        totalSent += sent;
		std::cout << "<" << fd << "> " << GREEN << ">> " << RESET << message << std::endl;
    }
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

void Server::handleTopic(Client* cli, const std::vector<std::string>& tokens)
{
    if (tokens.size() < 2)
    {
        sendToClient(*cli, ":localhost 461 " + cli->getNickname() + " TOPIC :Not enough parameters\r\n");
        return ;
    }

    std::string channelName = tokens[1];

    // Buscar canal en vector
    Channel* chan = 0;//NULL;
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
        sendToClient(*cli, ":localhost 403 " + cli->getNickname() + " " + channelName + " :No such channel\r\n");
        return ;
    }

    if (!chan->isMember(cli->getFd()))
    {
        sendToClient(*cli, ":localhost 442 " + cli->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return ;
    }

    // Solo consultar topic
    if (tokens.size() == 2)
    {
        if (chan->hasTopic())
        {
            sendToClient(*cli, ":localhost 332 " + cli->getNickname() + " " + channelName + " :" + chan->getTopic() + "\r\n");
        }
        else
        {
            sendToClient(*cli, ":localhost 331 " + cli->getNickname() + " " + channelName + " :No topic is set\r\n");
        }
        return ;
    }

    // Cambiar topic
    // Si modo +t está activo, solo operadores pueden
    if (chan->isModeT() && !chan->isOperator(cli->getFd()))
    {
        sendToClient(*cli, ":localhost 482 " + cli->getNickname() + " " + channelName + " :You're not channel operator\r\n");
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
        {
            sendToClient(*other, ":" + cli->getNickname() + " TOPIC " + channelName + " :" + topic + "\r\n");
        }
    }
}

void Server::handleMode(Client* cli, const std::vector<std::string>& tokens)
{
    if (tokens.size() < 2)
    {
        sendToClient(*cli, "461 MODE :Not enough parameters\r\n");
        return ;
    }

    // Comprobar que channel comienza por '#'
    std::string target = tokens[1];
    if (target[0] != '#')
    {
        sendToClient(*cli, "403 " + target + " :No such channel\r\n");
        return ;
    }

    // Buscar canal en el vector
    Channel* chan = 0;//NULL
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
        sendToClient(*cli, "403 " + target + " :No such channel\r\n");
        return ;
    }

    // Si no hay flags → mostrar modos actuales
    if (tokens.size() == 2)
    {
        std::string modes;
        //if (chan->isModeT()) modes += "t";
        if (modes.empty()) modes = "";
        //std::string modes = "";
        if (chan->isModeT()) modes += "t";
        if (chan->isModeI()) modes += "i";
        if (chan->isModeK()) modes += "k";
        if (chan->isModeL()) modes += "l";

        if (!modes.empty())
            sendToClient(*cli, ":" + cli->getNickname() + " MODE " + target + " +" + modes + "\r\n");
        else
            sendToClient(*cli, ":" + cli->getNickname() + " MODE " + target + " has no modes" + modes + "\r\n");
        return ;
    }

    // Validar que sea operador
    if (!chan->isOperator(cli->getFd()))
    {
        sendToClient(*cli, "482 " + target + " :You're not channel operator\r\n");
        return ;
    }

    // Procesar flags (+t / -t por ahora)
/*  std::string modes = tokens[2];
    for (size_t i = 0; i < modes.size(); ++i)
    {
        if (modes[i] == '+')
            continue ;
        if (modes[i] == '-')
            continue ;
        if (modes[i] == 't')
        {
            if (tokens[2][0] == '+')
                chan->setModeT(true);
            else if (tokens[2][0] == '-')
                chan->setModeT(false);

            // Avisar a todos en el canal
            const std::set<int>& members = chan->getClients();
            for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
            {
                Client* other = getClient(*it);
                if (other)
                {
                    sendToClient(*other, ":" + cli->getNickname() + " MODE " + target + " " + tokens[2] + "\r\n");
                }
            }
        }
        else
        {
            sendToClient(*cli, "472 " + std::string(1, modes[i]) + " :is unknown mode char\r\n");
        }
    }*/

    // Procesar flags
    std::string modeStr = tokens[2];
    bool add = true;
    size_t argIndex = 3; // índice para argumentos opcionales (key o limit)

    for (size_t i = 0; i < modeStr.size(); ++i)///////////FALTA CHECKEAR MODE == +-tikl
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
            sendToClient(*cli, "472 " + std::string(1, m) + " :is unknown mode char\r\n");
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
    }
}
