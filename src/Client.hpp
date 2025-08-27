#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <unistd.h>
#include <queue>

enum LoginStatus {
    NOT_AUTHENTICATED,
    PASS_OK,
    NICK_OK,
    USER_OK,
    REGISTERED
};


class Client {
private:
    int         _fd;
    std::string _ip;
    std::string _nickname;
    std::string _username; //<username> <hostname> <servername> <realname>
	LoginStatus _status; // 0=no autenticado, 1=pass ok, 2=nick ok, 3=user ok, 4=autenticado
    std::string _buffer;  // buffer acumulativo de datos recibidos

	// En IRC el handshake inicial (PASS + NICK + USER) es obligatorio antes del protocolo IRC
	
public:
    Client() : _fd(-1), _status(NOT_AUTHENTICATED) {}
    Client(int fd) : _fd(fd), _status(NOT_AUTHENTICATED) {}

    // --- Getters / Setters básicos ---
    int getFd() const { return _fd; }
    void setFd(int fd) { _fd = fd; }

    const std::string& getIp() const { return _ip; }
    void setIp(const std::string& ip) { _ip = ip; }

    const std::string& getNick() const { return _nickname; }
    void setNick(const std::string& nick) { _nickname = nick; }

    const std::string& getUser() const { return _username; }
    void setUser(const std::string& user) { _username = user; }

	LoginStatus getStatus() const { return _status; }
    void setStatus(LoginStatus s) { _status = s; }

	
    // --- Buffer de entrada ---
    void appendToBuffer(const std::string& data) {
        _buffer += data;
        // Si quisieras protegerte de flood:
        if (_buffer.size() > 4096) {  // ejemplo: límite de 8 comandos (~512*8)
            _buffer.clear(); // o desconectar cliente
        }
    }

    std::string& getBuffer() { return _buffer; }
    void clearBuffer() { _buffer.clear(); }
};

#endif

