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
#include "Client.hpp"
#include "Channel.hpp" 

class Server {
private:
    int _port;
    const std::string _password; // no sera modificable
    int _server_fd;
    std::vector<struct pollfd> _fds;
    std::map<int, Client> _clients;       // fd -> Client
    std::map<std::string, Channel> _channels; // nombre -> Channel
    struct sockaddr_in _server_addr;


    void initSocket();
    void handleNewConnection();
    void handleClientMessage(size_t i);

public:
    Server(int port, const std::string& password);
    ~Server();

    void run();
};

#endif
