#include "../include/Channel.hpp"

Channel::Channel(const std::string& name)
:   _name(name),
    _topic(""),
    _key(""),
    _userLimit(0),
    _modeT(false),
    _modeI(false),
    _modeK(false),
    _modeL(false) {}

const std::string& Channel::getName() const { return _name; }
const std::string& Channel::getTopic() const { return _topic; }

void Channel::setTopic(const std::string& topic) { _topic = topic; }
bool Channel::hasTopic() const { return !_topic.empty(); }

// --- modos ---
// +t
void Channel::setModeT(bool enabled) { _modeT = enabled; }
bool Channel::isModeT() const { return _modeT; }

// +i
void Channel::setModeI(bool enabled) { _modeI = enabled; }
bool Channel::isModeI() const { return _modeI; }

// +k
void Channel::setModeK(bool enabled) { _modeK = enabled; }
bool Channel::isModeK() const { return _modeK; }

void Channel::setKey(const std::string& key) {
    _key = key;
    _modeK = true;
}

const std::string& Channel::getKey() const {
    return _key;
}

// +l
void Channel::setModeL(bool enabled) { _modeL = enabled; }
bool Channel::isModeL() const { return _modeL; }

void Channel::setUserLimit(int limit) {
    _userLimit = limit;
    _modeL = true;
}

int Channel::getUserLimit() const {
    return _userLimit;
}

bool Channel::addClient(int fd, bool isOperator)
{
    if (_modeL && (int)_clients.size() >= _userLimit)
    {
        return false; // canal lleno
    }

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