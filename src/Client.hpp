#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    int _fd;
    std::string _nickname;
    std::string _username;
    std::string _buffer;

public:
    Client();
    Client(int fd);
    ~Client();

    int getFd() const;
    std::string getNickname() const;
    void setNickname(const std::string& nick);

    void appendBuffer(const std::string& data);
    std::string extractMessage(); // obtiene línea completa
};

#endif

