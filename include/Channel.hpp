#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel {
private:
    std::string     _name;
    std::set<int>   _clients;   // fds de clientes
    std::set<int>   _operators; // fds de operadores

public:
    Channel(const std::string& name);

    const std::string& getName() const;

    // gestión de miembros
    bool addClient(int fd, bool isOperator = false);
    void removeClient(int fd);
    bool isMember(int fd) const;
    bool isOperator(int fd) const;

    const std::set<int>& getClients() const;
};

#endif