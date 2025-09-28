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


//Busca o crea canal si no existe
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






