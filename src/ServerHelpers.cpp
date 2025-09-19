#include "Server.hpp"


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



Client *Server::getClient(int fd)
{
	for (size_t i = 0; i < _clients.size(); i++)
    {
		if (_clients[i].getFd() == fd)
			return &_clients[i];
	}
	return NULL;
}



