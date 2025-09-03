#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel {
private:
    std::string     _name;
    std::string     _topic;
    std::string     _key;
    int             _userLimit;
    bool            _modeT;     // si está activo, solo operadores pueden cambiar el topic
    bool            _modeI;     // Invite Only (+/- i)
    bool            _modeK;     // Channel Key (+/- k)
    bool            _modeL;     // User Limit (+/- l)
    std::set<int>   _clients;   // fds de clientes
    std::set<int>   _operators; // fds de operadores
//  std::set<int>   _invited; // opcional, para manejar a quién se ha invitado

public:
    Channel(const std::string& name);

    // Getters and Setters
    const std::string&  getName() const;

    const std::string&  getTopic() const;
    void                setTopic(const std::string& topic);
    bool                hasTopic() const;

    void                setModeT(bool enabled);
    bool                isModeT() const;

    const std::string&  getKey() const;
    void                setKey(const std::string& key);

    // Nuevos getters/setters
    void                setModeI(bool enabled);
    bool                isModeI() const;

    void                setModeK(bool enabled);
    bool                isModeK() const;
    

    void                setModeL(bool enabled);
    bool                isModeL() const;

    void                setUserLimit(int limit);
    int                 getUserLimit() const;

    // gestión de miembros
    bool addClient(int fd, bool isOperator = false);
    void removeClient(int fd);
    bool isMember(int fd) const;
    bool isOperator(int fd) const;

    const std::set<int>& getClients() const;
};

#endif