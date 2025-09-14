#include "Server.hpp"

bool Server::_signalFlag = false;

Server::Server() : _port(0), _serverFd(-1) {}

void Server::signalHandler(int signum)
{
    (void)signum;
    std::cout << std::endl << "Signal Received!" << std::endl;
    Server::_signalFlag = true;
}

void Server::removeEmptyChannels()
{
    for (size_t i = 0; i < _channels.size(); )
    {
        if (_channels[i].getClients().empty())
        {
            std::cout << "Eliminando canal vacío: " << _channels[i].getName() << std::endl;
            _channels.erase(_channels.begin() + i);
            // No incrementamos i, porque tras erase el siguiente canal pasa a ocupar esa posición
        }
        else
            ++i;
    }
}

void Server::clearClient(int fd)
{
    for (size_t i = 0; i < _fds.size(); i++)
    {
        if (_fds[i].fd == fd)
        {
            _fds.erase(_fds.begin() + i);
            break ;
        }
    }
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i].getFd() == fd)
        {
            _clients.erase(_clients.begin() + i);
            break ;
        }
    }
    for (size_t i = 0; i < _channels.size(); i++)
    {   
        if (_channels[i].isMember(fd))
        {
            _channels[i].removeClient(fd);
        }
    }
    removeEmptyChannels();
}


void Server::closeFds()
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        std::cout << RED << "Client <" << _clients[i].getFd() << "> Disconnected" << WHI << std::endl;
        close(_clients[i].getFd());
    }
    if (_serverFd != -1)
    {
        std::cout << RED << "Server <" << _serverFd << "> Disconnected" << WHI << std::endl;
        close(_serverFd);
    }
}

void Server::acceptNewClient()
{
    Client cli;
    struct sockaddr_in cliAdd;
    struct pollfd newPoll;
    socklen_t len = sizeof(cliAdd);

    int newFd = accept(_serverFd, (sockaddr *)&cliAdd, &len);
    if (newFd == -1)
    {
        std::cout << "accept() failed" << std::endl;
        return ;
    }

    if (fcntl(newFd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cout << "fcntl() failed" << std::endl;
        return ;
    }

    newPoll.fd = newFd;
    newPoll.events = POLLIN;
    newPoll.revents = 0;

    cli.setFd(newFd);
    cli.setIp(inet_ntoa(cliAdd.sin_addr));
    _clients.push_back(cli);
    _fds.push_back(newPoll);

    std::cout << GRE << "Client <" << newFd << "> Connected" << WHI << std::endl;
}

void Server::createSocket()
{
    int en = 1;
    struct sockaddr_in add;
    struct pollfd newPoll;

    add.sin_family = AF_INET;
    add.sin_addr.s_addr = INADDR_ANY;
    add.sin_port = htons(this->_port);

    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd == -1)
        throw(std::runtime_error("faild to create socket"));

    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1)
        throw(std::runtime_error("faild to set option (SO_REUSEADDR) on socket"));

    if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) == -1)
        throw(std::runtime_error("faild to set option (O_NONBLOCK) on socket"));

    if (bind(_serverFd, (struct sockaddr *)&add, sizeof(add)) == -1)
        throw(std::runtime_error("faild to bind socket"));

    if (listen(_serverFd, SOMAXCONN) == -1)
        throw(std::runtime_error("listen() failed"));

    newPoll.fd = _serverFd;
    newPoll.events = POLLIN;
    newPoll.revents = 0;
    _fds.push_back(newPoll);
}

void Server::serverInit(int port, std::string& pwd)
{
    this->_port = port;
    this->_password = pwd;
    createSocket();

    std::cout << GRE << "Server <" << _serverFd << "> Connected" << WHI << std::endl;
    std::cout << "Waiting to accept a connection...\n";

    while (Server::_signalFlag == false)
    {
        if ((poll(&_fds[0], _fds.size(), -1) == -1) && Server::_signalFlag == false)
            throw(std::runtime_error("poll() failed"));

        for (size_t i = 0; i < _fds.size(); i++)
        {
            if (_fds[i].revents & POLLIN)
            {
                if (_fds[i].fd == _serverFd)
                    acceptNewClient();
                else
                    receiveNewData(_fds[i].fd);
            }
        }
    }
    closeFds();
}

/************************* New Functions **************************/

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
        std::cout << RED << "Client <" << fd << "> Disconnected" << WHI << std::endl;
        clearClient(fd);
        close(fd);
        return ;
    }

    Client* cli = getClient(fd);
    if (!cli)
        return ;
    // Añadimos los nuevos datos de buff en el buffer del cliente
    cli->appendToBuffer(std::string(buff, bytes));
    std::string& buffer = cli->getBuffer();
    size_t pos;

    // Procesar solo cuando haya una línea completa acabada en /r/n
    while ((pos = buffer.find("\r\n")) != std::string::npos)
    {
        std::string command = buffer.substr(0, pos);  // comando completo
        buffer.erase(0, pos + 2);                     // quitamos comando\r\n del buffer

        if (!command.empty())
        {
            std::cout << YEL << "Client <" << fd << "> Command: " << WHI << command << std::endl;
            parseCommand(cli, command); // <--NO BORRAR CLIENTES DESDE LOS HANDLERS
            // O FALLARÁ AL VOLVER A ENTRAR AL WHILE Y LLAMAR A buffer.find
            // YA QUE buffer ES UNA REFERENCIA AL _buffer DEL CLIENTE BORRADO!!!!!!!!!!!
        }
    }
}

// Helper para dividir un string en tokens
std::vector<std::string> split(const std::string& str, char delim)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim))
        tokens.push_back(token);
    return tokens;
}

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

    // 1. Separar en tokens por espacio
    std::vector<std::string> tokens = split(cmd, ' ');
    if (tokens.empty())
        return ;

    std::string command = tokens[0];

    // Convertir a mayúsculas por seguridad
    for (size_t i = 0; i < command.size(); ++i)
        command[i] = std::toupper(command[i]);
    
    // 2. Enrutamos según comando
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
    else if (command == "KICK") {
        handleKick(cli, tokens);
    }
    else if (command == "INVITE") {
        handleInvite(cli, tokens);
    }
    else {
        // std::cout << RED << "Unknown command: " << cmd << WHI << std::endl;
        return ;
    }
}

void Server::handlePass(Client* cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;

    std::string target;
    if (cli->getNickname().empty())
        target = "*";
    else
        target = cli->getNickname();

    if (tokens.size() < 2) // 461: faltan parámetros
    {
        sendToClient(*cli, ":ircserv 461 " + target + " PASS :Not enough parameters\r\n");
        return ;
    }

    if (cli->isAuthenticated())// 462: ya registrado
    {
        sendToClient(*cli, ":ircserv 462 " + target + " :You may not reregister\r\n");
        return ;
    }

    // Extraer la pass (puede venir con ':')
    std::string pwd = tokens[1];
    if (!pwd.empty() && pwd[0] == ':')
        pwd.erase(0, 1);

    if (_password != pwd)    // 464: password incorrecta → enviar error y NO cerrar conexión
    {
        cli->setPassAccepted(false);
        sendToClient(*cli, ":ircserv 464 " + target + " :Password incorrect\r\n");
        return ;
    }
    cli->setPassAccepted(true);    // Password correcta → marcar como aceptada
    sendToClient(*cli, ":ircserv NOTICE AUTH :Password accepted\r\n");
}

void Server::handleNick(Client* cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;
    
    std::string target;
    if (cli->getNickname().empty())
        target = "*";
    else
        target = cli->getNickname();
    
    if (!cli->hasPassAccepted())
    {
        sendToClient(*cli, ":ircserv 464 " + target + " :Password incorrect\r\n");
        return ;
    }

    if (tokens.size() < 2)// 431: no se ha dado nick
    {
        sendToClient(*cli, ":ircserv 431 " + target + " :No nickname given\r\n");
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

    if (getClientByNick(newNick))// 433: nick ya en uso
    {
        sendToClient(*cli, ":ircserv 433 * " + newNick + " :Nickname is already in use\r\n");
        return ;
    }

    cli->setNickname(newNick);// Asignar nick
// Aquí no se autentica todavía → eso ocurrirá en handleUser()
    sendToClient(*cli, ":ircserv NOTICE AUTH :Nickname set to " + newNick + "\r\n");
}

void Server::handleUser(Client* cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;

    std::string target;
    if (cli->getNickname().empty())
        target = "*";
    else
        target = cli->getNickname();

    if (!cli->hasPassAccepted())
    {
        sendToClient(*cli, ":ircserv 464 " + target + " :Password incorrect\r\n");
        return ;
    }
    
    if (cli->getNickname().empty())
    {
        sendToClient(*cli, ":ircserv 431 " + target + " :No nickname given\r\n");
        return ;
    }

    // USER <username> <mode> <unused> :<realname with spaces>
    if (tokens.size() < 4)    // 461: faltan parámetros
    {
        sendToClient(*cli, ":ircserv 461 " + target + " USER :Not enough parameters\r\n");
        return ;
    }

    if (cli->isAuthenticated())// 462: ya registrado
    {
        sendToClient(*cli, ":ircserv 462 " + target + " :You may not reregister\r\n");
        return ;
    }

    cli->setUsername(tokens[1]);

    // Realname puede ser tokens[3] o empezar en tokens[3]+tokens[4]+...
    std::string realname;
    if (tokens.size() >= 5)
        realname = joinFrom(tokens, 4, " ");
    else
        realname = tokens[3];
    if (!realname.empty() && realname[0] == ':')
        realname.erase(0, 1);

    cli->setRealname(realname);

    tryRegister(*cli);
}

void Server::handleJoin(Client *cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;
    
    std::string target;
    if (cli->getNickname().empty())
        target = "*";
    else
        target = cli->getNickname();

    if (!cli->isAuthenticated())
    {
        sendToClient(*cli, ":ircserv 451 " + target + " :You have not registered\r\n");
        return ;
    }
    
    if (tokens.size() < 2)
    {
        sendToClient(*cli, "461 JOIN :Not enough parameters\r\n");
        return ;
    }

    std::string channelName = tokens[1];
    std::string keyArg; // clave opcional
    if (tokens.size() >= 3)
        keyArg = tokens[2]; // el cliente puede enviar la key

    if (channelName[0] != '#')/////Añadir restricciones!!!!!! (caracteres, longitud, etc)
    {
        sendToClient(*cli, "479 " + channelName + " :Illegal channel name\r\n");
        return ;
    }

    Channel* chan = getOrCreateChannel(channelName);

    // 1. Comprobar invite-only (+i)
    if (chan->isModeI() && !(chan->isOperator(cli->getFd()) || chan->isInvited(cli->getFd())))
    {
        sendToClient(*cli, "473 " + channelName + " :Cannot join channel (+i)\r\n");
        return ;
    }
    // 2. Comprobar clave (+k)
    if (chan->isModeK())
    {
        if (keyArg.empty() || keyArg != chan->getKey())
        {
            sendToClient(*cli, "475 " + channelName + " :Cannot join channel (+k)\r\n");
            return ;
        }
    }
    // 3. Comprobar límite de usuarios (+l)
    if (chan->isModeL() && static_cast<int>(chan->getClients().size()) >= chan->getUserLimit())
    {
        sendToClient(*cli, "471 " + channelName + " :Cannot join channel (+l)\r\n");
        return ;
    }
    // 4. Primer usuario → operador
    bool isFirst = chan->getClients().empty();
    chan->addClient(cli->getFd(), isFirst);
    // Si el canal es +i y el cliente estaba invitado → eliminar invitación
    if (chan->isModeI() && chan->isInvited(cli->getFd()))
        chan->removeInvite(cli->getFd());

    // Aviso al propio cliente
    sendToClient(*cli, ":" + cli->getNickname() + " JOIN " + channelName + "\r\n");
    // Aviso a los demás en el canal
    const std::set<int>& members = chan->getClients();
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        if (*it != cli->getFd())
        {
            Client *other = getClient(*it);
            if (other)
                sendToClient(*other, ":" + cli->getNickname() + " JOIN " + channelName + "\r\n");
        }
    }
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

    if (!cli->isAuthenticated())
    {
        sendToClient(*cli, ":ircserv 451 " + target + " :You have not registered\r\n");
        return ;
    }
    
    // 1. Validar parámetros
    if (tokens.size() < 3)
    {
        sendToClient(*cli, ":localhost 461 " + cli->getNickname() + " PRIVMSG :Not enough parameters\r\n");
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
                    sendToClient(*other, ":" + cli->getNickname() + "!" + cli->getUsername() + "@localhost" + " PRIVMSG " + target + " :" + message + "\r\n");
            }
        }
    }
    else    // --- PRIVMSG a usuario ---
    {
        Client* targetClient = getClientByNick(target);
        if (!targetClient)
        {
            sendToClient(*cli, ":localhost 401 " + cli->getNickname() + " " + target + " :No such nick\r\n");
            return ;
        }
        sendToClient(*targetClient, ":" + cli->getNickname() + "!" + cli->getUsername() + "@localhost" + " PRIVMSG " + target + " :" + message + "\r\n");
    }
}

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
                            " ircserv 1.0.0 io imnpst bkloveqjfI\r\n");

    // 005 RPL_ISUPPORT (puedes añadir más líneas si quieres)
    sendToClient(client, ":ircserv 005 " + nick +
                            " CHANTYPES=# PREFIX=(ov)@+ NICKLEN=30 MAXCHANNELS=20 :are supported by this server\r\n");
}

void Server::tryRegister(Client& client)
{
    if (client.isAuthenticated())
        return;

    if (client.hasPassAccepted() && !client.getNickname().empty() && !client.getUsername().empty())
    {
        client.setAuthenticated(true);
        sendWelcomeMessages(client);

        std::cout << GRE << "Client <" << client.getFd() << "> authenticated as "
                  << client.getNickname() << WHI << std::endl;
    }
    else
        sendToClient(client, ":ircserv 451 * :You have not registered\r\n");
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
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // El socket no está listo → esperar y reintentar
                break ;
            }
            else
            {
                std::cerr << RED << "Error sending to client <" << fd << ">" << WHI << std::endl;
                clearClient(fd);
                close(fd);
                return ;
            }
        }
        totalSent += sent;
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
    if (cli->getNickname().empty())
        target = "*";
    else
        target = cli->getNickname();

    if (!cli->isAuthenticated())
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
    if (cli->getNickname().empty())
        target = "*";
    else
        target = cli->getNickname();

    if (!cli->isAuthenticated())
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
    if (cli->getNickname().empty())
        target = "*";
    else
        target = cli->getNickname();

    if (!cli->isAuthenticated())
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
    removeEmptyChannels();
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
