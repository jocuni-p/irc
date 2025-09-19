#include "Server.hpp"


void Server::acceptNewClient()
{
    struct sockaddr_in cliAdd;
    socklen_t len = sizeof(cliAdd);
    int newFd = accept(_serverFd, (sockaddr *)&cliAdd, &len);
    if (newFd == -1) { // si accept da -1 => no hay nada ahora, pues salgo de la funcion
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return; 
		}
		std::perror("(!) ERROR:  accept()");
        return; // no continua con fd invalido
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





void Server::receiveNewData(int fd)
{
    char buff[1024];
    memset(buff, 0, sizeof(buff));

    ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);

    if (bytes <= 0)
    {
        clearClient(fd);
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
	
    // Procesar solo cuando haya una linea completa
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



void Server::sendToClient(Client& client, const std::string& message)
{
    if (client.isMarkedForRemoval()) return; // Si el cliente se ha de eliminar, no enviarle nada

    int fd = client.getFd();
    const char* buffer = message.c_str();
    size_t totalSent = 0;
    size_t length = message.size();

    while (totalSent < length)
    {
        ssize_t sent = send(fd, buffer + totalSent, length - totalSent, 0);
        if (sent <= 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) break; // El socket no esta listo → esperar y reintentar
			clearClient(fd);
            return;
        }
        totalSent += sent;
    }
	// Muestra mensaje en consola sin el '\r\n' 
	std::cout << "<" << fd << "> " << GREEN << ">> " 
              << RESET << Utils::stripCRLF(message) << std::endl;
}


