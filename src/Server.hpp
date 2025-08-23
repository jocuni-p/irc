#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <map>
#include <vector>
#include <string>  //???
#include <cstdlib>
#include <stdexcept>
#include <poll.h>
#include <netinet/in.h>
//#include "Client.hpp"
#include "Channel.hpp"

class Client; // Incluyo la clase aqui o arriba?????

class Server {
private:
    const int _port; // no modificable
    const std::string _password; // no sera modificable
    int _server_fd; // no modificable
    std::vector<struct pollfd> _fds;
    std::map<int, Client> _clients;       // fd -> Client
    std::map<std::string, Channel> _channels; // nombre -> Channel
    struct sockaddr_in _server_addr; //struct del sistema, contiene IP y puerto del socket


    void initSocket();
    void handleNewConnection();
    void handleClientMessage(size_t i);

public:
    Server(int port, const std::string& password);
    ~Server();

    void run();

	static bool isValidPasswordArg(const std::string &pass);
};

#endif
