#include "Server.hpp"



void Server::handleJoin(Client* cli, const std::vector<std::string>& tokens)
{

    if (tokens.size() < 2)
    {
        sendToClient(*cli, ":ircserv 461 JOIN :Not enough parameters\r\n");
        return ;
    }

    std::string channelName = tokens[1];
    std::string keyArg; // clave opcional
    if (tokens.size() >= 3)
        keyArg = tokens[2]; // el cliente puede enviar la key

    if (channelName[0] != '#')
    {
        sendToClient(*cli, ":ircserv 479 " + channelName + " :Illegal channel name\r\n");
        return ;
    }

    Channel* chan = getOrCreateChannel(channelName);

    // Comprobar invite-only (+i)
    if (chan->isModeI() && !chan->isOperator(cli->getFd()) && !chan->isInvited(cli->getFd()))
    {
        sendToClient(*cli, ":ircserv 473 " + channelName + " :Cannot join channel (+i)\r\n");
        return ;
    }

    // Comprobar clave (+k)
    if (chan->isModeK())
    {
        if (keyArg.empty() || keyArg != chan->getKey())
        {
            sendToClient(*cli, ":ircserv 475 " + channelName + " :Cannot join channel (+k)\r\n");
            return ;
        }
    }

    // Comprobar limite de usuarios (+l)
    if (chan->isModeL() && static_cast<int>(chan->getClients().size()) >= chan->getUserLimit())
    {
        sendToClient(*cli, ":ircserv 471 " + channelName + " :Cannot join channel (+l)\r\n");
        return ;
    }

    // Primer usuario → operador
    bool isFirst = chan->getClients().empty();
    chan->addClient(cli->getFd(), isFirst);

	chan->removeInvite(cli->getFd());


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



//Periodicamente HexChat lanza un WHO para actualizar lista de clientes del canal
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
              << "\r\n";

        sendToClient(*cli, reply.str());
    }

    // Fin de WHO list
    std::ostringstream end;
    end << ":" << "ircserv"
        << " 315 " << cli->getNickname()
        << " " << channelName << " :End of WHO list\r\n";
    sendToClient(*cli, end.str());
}


void Server::handlePrivmsg(Client *cli, const std::vector<std::string>& tokens)
{
    if (!cli)
        return;

    std::string sender;
    if (cli->getNickname().empty())
        sender = "*";
    else
        sender = cli->getNickname();

    // Validar parametros
    if (tokens.size() < 3)
    {
        sendToClient(*cli, ":ircserv 461 " + cli->getNickname() + " PRIVMSG :Not enough parameters\r\n");
        return;
    }

    // Construir mensaje completo
    std::string message = tokens[2];
    if (tokens[2][0] == ':')
        message = tokens[2].substr(1);
    for (size_t i = 3; i < tokens.size(); ++i)
        message += " " + tokens[i];

    // Separar lista de destinatarios por comas
    std::vector<std::string> targets = Utils::split(tokens[1], ',');
    for (size_t t = 0; t < targets.size(); ++t)
    {
        std::string dest = targets[t];
        if (dest.empty())
            continue;

        // Destino es canal
        if (dest[0] == '#')
        {
            Channel *chan = NULL;
            for (size_t i = 0; i < _channels.size(); ++i)
            {
                if (_channels[i].getName() == dest)
                {
                    chan = &_channels[i];
                    break;
                }
            }

            if (!chan)
            {
                sendToClient(*cli, ":ircserv 403 " + cli->getNickname() + " " + dest + " :No such channel\r\n");
                continue;
            }

            if (!chan->isMember(cli->getFd()))
            {
                sendToClient(*cli, ":ircserv 442 " + cli->getNickname() + " " + dest + " :You're not on that channel\r\n");
                continue;
            }

            // reenviar a todos los clientes del canal excepto el emisor
            const std::set<int>& members = chan->getClients();
            for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
            {
                if (*it != cli->getFd())
                {
                    Client* other = getClient(*it);
                    if (other)
                        sendToClient(*other, ":" + cli->getNickname() + "!" + cli->getUsername() +
                                               "@localhost PRIVMSG " + dest + " :" + message + "\r\n");
                }
            }
        }
        // Destino es usuario
        else
        {
            Client* targetClient = getClientByNick(dest);
            if (!targetClient)
            {
                sendToClient(*cli, ":ircserv 401 " + cli->getNickname() + " " + dest + " :No such nick\r\n");
                continue;
            }
            sendToClient(*targetClient, ":" + cli->getNickname() + "!" + cli->getUsername() +
                                       "@localhost PRIVMSG " + dest + " :" + message + "\r\n");
        }
    }
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

    // Solo consultar topic
    if (tokens.size() == 2)
    {
        if (chan->hasTopic())
            sendToClient(*cli, ":ircserv 332 " + cli->getNickname() + " " + channelName + " :" + chan->getTopic() + "\r\n");
        else
            sendToClient(*cli, ":ircserv 331 " + cli->getNickname() + " " + channelName + " :No topic is set\r\n");
        return ;
    }
    // Cambiar topic (Si modo +t esta activo, solo operadores pueden)
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

    // Caso 2: modificacion de modos
    if (!checkOperator(cli, chan, target))
        return;

    applyChannelModes(cli, chan, tokens, target);
}


