#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <unistd.h>

class Client {
private:
    int _fd;
    std::string _nickname;
    std::string _username;
    std::string _buffer; //debo delimitar su tamanyo ??

public:
//    Client(); Diria que no sera necesario porque no lo usare nunca vacio
    Client(int fd);
    ~Client();

    int getFd() const; // no se modificaran los datos miembros del obj
    std::string getNickname() const;
    void setNickname(const std::string &nick);

	// Creo que no necesito get/set para username (en principio deberia estar oculto)

    void appendBuffer(const std::string &data); // guarda los datos recibidos
    std::string extractMessage(); // obtiene línea hasta \r\n y la elimina del buffer
};

#endif

