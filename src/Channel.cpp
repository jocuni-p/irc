#include "../include/Channel.hpp"

Channel::Channel(const std::string& name) : _name(name) {}

const std::string& Channel::getName() const { return _name; }

bool Channel::addClient(int fd, bool isOperator)
{
    _clients.insert(fd);
    if (isOperator)
        _operators.insert(fd);
    return true;
}

void Channel::removeClient(int fd)
{
    _clients.erase(fd);
    _operators.erase(fd);
}

bool Channel::isMember(int fd) const
{
    return _clients.find(fd) != _clients.end();
}

bool Channel::isOperator(int fd) const
{
    return _operators.find(fd) != _operators.end();
}

const std::set<int>& Channel::getClients() const
{
    return _clients;
}