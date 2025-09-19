#include "Server.hpp"


// Proceso de autenticacion del cliente
void Server::handshake(Client *cli, const std::string &cmd)
{
	std::vector<std::string> tokens = Utils::split(cmd, ' ');
	if (tokens.empty())
		return;

	std::string command = tokens[0];

	//Convertir a mayusculas por seguridad
	for (size_t i = 0; i < command.size(); ++i)
	{
		command[i] = std::toupper(command[i]);
	}

	if (command == "CAP" && cli->getStatus() == NOT_AUTHENTICATED)
	{
		handleCap(cli);
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
	else if (command == "CAP END")
		return;
	else
	{
		sendToClient(*cli, "451 :You have not registered\r\n");
	}

	// Si el cliente llega a USER_OK → lo marcamos como AUTHENTICATED
	if (cli->getStatus() == USER_OK)
	{
		cli->setStatus(AUTHENTICATED);
		std::cout << YELLOW_PALE << "<" << cli->getFd() << "> " 
					<< " Authenticated " << cli->getNickname() << RESET << std::endl;
		sendToClient(*cli, ":ircserv 001 " 	+ cli->getNickname() 
											+ " :Welcome to the IRC server " 
											+ cli->getNickname() + "!\r\n");


	}
}




//RESPUESTA SOBRE LA LISTA DE CAPACIDADES QUE SOPORTA EL SERVER
void Server::handleCap(Client *cli)
{
	// Respondemos con lista vacia de capabilities (:)
	std::ostringstream reply;
	std::string nick = cli->getNickname().empty() ? "nickname" : cli->getNickname();
	reply << ":Server CAP "
		  << nick
		  << " LS :\r\n";

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
		return ;
	}

	if (cli->getStatus() == AUTHENTICATED) { // Si ya estaba authenticado previamente
		sendToClient(*cli, ":ircserv 462 " + target + " :You may not reregister\r\n");
	 	return ;
	}

	const std::string& passArg = tokens[1];
	std::string pwd = passArg;

	// Elimina ':' si preceden a la password
	if (!pwd.empty() && pwd[0] == ':')
		pwd.erase(0, 1);

	if (_password != pwd) {
		sendToClient(*cli, ":ircserv 464 " + target + " :Password incorrect\r\n");
		return;
	}

	cli->setStatus(PASS_OK);
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

	cli->setNickname(newNick);
	sendToClient(*cli, ":* NICK " + newNick + "\r\n");

	if (cli->getStatus() == NICK_AGAIN) {
		cli->setStatus(USER_OK);
	} else {
		cli->setStatus(NICK_OK);
	}
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

	// USER <username> <mode> <unused> :<realname with spaces>
	if (tokens.size() < 4)
	{
		sendToClient(*cli, ":ircserv 461 " + target + " USER :Not enough parameters\r\n");
		return ;
	}

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
}
