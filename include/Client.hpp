#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
private:
    int             _fd;
    std::string     _ip;       
    std::string     _nickname;
    std::string     _username;
    std::string     _realname;
    std::string     _buffer;
    bool            _passAccepted;
    bool            _authenticated;

public:
    Client();
   // Client(int fd);
    ~Client();

    // --- Getters ---
    int             getFd() const;
    std::string     getIp() const;
    std::string     getNickname() const;
    std::string     getUsername() const;
    std::string     getRealname() const;
    bool            isAuthenticated() const;
    std::string&    getBuffer();

    // --- Setters ---
    void            setFd(int newFd);
    void            setIp(const std::string& newIp);
    void            setNickname(const std::string& nick);
    void            setUsername(const std::string& user);
    void            setRealname(const std::string& real);
    void            setAuthenticated(bool status);

    // PASS
    bool            hasPassAccepted() const;
    void            setPassAccepted(bool status);

    // --- Buffer handling ---
    void            appendToBuffer(const std::string& data);
    void            clearBuffer();
};

#endif
