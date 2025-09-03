#include "Server.hpp"

bool Server::_signalFlag = false;

Server::Server() : _port(0), _serverFd(-1) {}

void Server::signalHandler(int signum)
{
    (void)signum;
    std::cout << std::endl << "Signal Received!" << std::endl;
    Server::_signalFlag = true;
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

/*void Server::receiveNewData(int fd)
{
    char buff[1024];
    memset(buff, 0, sizeof(buff));

    ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);

    if (bytes <= 0)
    {
        std::cout << RED << "Client <" << fd << "> Disconnected" << WHI << std::endl;
        clearClient(fd);
        close(fd);
    } else
    {
        buff[bytes] = '\0';
        std::cout << YEL << "Client <" << fd << "> Data: " << WHI << buff;
        // Aquí luego usaremos el buffer de Client para parsear comandos
    }
}*/

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

/*
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

    // Guardamos los nuevos datos en el buffer del cliente
    Client* cli = getClient(fd);
    if (!cli)
        return ;

    cli->appendToBuffer(std::string(buff, bytes));

    // Procesar comandos completos (separados por \r\n)
    std::string& buffer = cli->getBuffer();
    size_t pos;

    // Procesar solo cuando haya una línea completa
    while ((pos = buffer.find("\r\n")) != std::string::npos)
    {
        std::string command = buffer.substr(0, pos);
        buffer.erase(0, pos + 2); // eliminar hasta \r\n

        if (!command.empty())
        {
            std::cout << YEL << "Client <" << fd << "> Command: " << WHI << command << std::endl;

            // Aquí más adelante llamaremos a un parser de comandos
            // parseCommand(cli, command);
            parseCommand(cli, command);
        }
    }
}
*/
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
        return;
    }

    // Guardamos los nuevos datos en el buffer del cliente
    Client* cli = getClient(fd);
    if (!cli)
        return;

    cli->appendToBuffer(std::string(buff, bytes));
    std::string& buffer = cli->getBuffer();
    size_t pos;

    // Procesar solo cuando haya una línea completa
    while ((pos = buffer.find("\r\n")) != std::string::npos)
    {
        std::string command = buffer.substr(0, pos);  // comando completo
        buffer.erase(0, pos + 2);                     // quitamos del buffer

        if (!command.empty())
        {
            std::cout << YEL << "Client <" << fd << "> Command: " << WHI << command << std::endl;
            parseCommand(cli, command); // <-- ahora siempre es una línea completa
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
    {
        command[i] = std::toupper(command[i]);
        //command[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(command[i])));//uppercase seguro

    }

    // 2. Enrutamos según comando
    if (command == "CAP" || command == "QUIT") {
        return ;
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
    else {
        std::cout << RED << "Unknown command: " << cmd << WHI << std::endl;
        // Aquí luego podemos enviar un error al cliente
        //return ;
    }
}


/*void Server::handlePass(Client *cli, const std::vector<std::string>& tokens)
{
    (void)cli;
    std::cout << tokens[0] << " command received" << std::endl;
}*/

/*void Server::handleNick(Client* cli, const std::vector<std::string>& tokens)
{
    (void)cli;
    std::cout << tokens[0] << " command received" << std::endl;
}*/

/*void Server::handleUser(Client* cli, const std::vector<std::string>& tokens)
{
    (void)cli;
    std::cout << tokens[0] << " command received" << std::endl;
}*/

void Server::handlePass(Client* cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return ;

    if (tokens.size() < 2)
    {
        sendToClient(*cli, "461 PASS :Not enough parameters\r\n");
        return ;
    }
    if (cli->isAuthenticated())
    {
        sendToClient(*cli, "462 :You may not reregister\r\n");
        return ;
    }

    const std::string& passArg = tokens[1]; // puede venir con ':' delante en algunos clientes
    std::string pwd = passArg;
    if (!pwd.empty() && pwd[0] == ':')
        pwd.erase(0, 1);

    if (_password != pwd) {
        sendToClient(*cli, "464 :Password incorrect\r\n");
        return ;
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


/*void Server::handleJoin(Client* cli, const std::vector<std::string>& tokens)
{
    (void)cli;
    std::cout << tokens[0] << " command received" << std::endl;
}*/
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
}

/*void Server::handlePrivmsg(Client* cli, const std::vector<std::string>& tokens)
{
    (void)cli;
    std::cout << tokens[0] << " command received" << std::endl;
}*/
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

        std::cout << GRE << "Client <" << client.getFd() << "> authenticated as " 
                  << client.getNickname() << WHI << std::endl;
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
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // El socket no está listo → esperar y reintentar
                break ;
            }
            else
            {
                std::cerr << RED << "Error sending to client <" << fd << ">" << WHI << std::endl;
                //removeClient(fd); // cerrar la conexión
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