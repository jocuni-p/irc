#ifndef SERVER_HPP
#define SERVER_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cctype> // para toupper
#include <cerrno>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <string>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <map>
#include "Channel.hpp"
#include "Client.hpp"
#include "Utils.hpp"

//MOVIDO A Utils.hpp
// #define RESET   "\033[0m"
// #define RED     "\033[31m"
// #define GREEN   "\033[32m"
// #define YELLOW  "\033[33m"
// #define BLUE    "\033[34m"
// #define MAGENTA "\033[35m"
// #define CYAN    "\033[36m"
// #define WHITE   "\033[37m"
// // #define RED "\e[1;31m"
//  #define WHI "\e[0;37m"
//  #define GRE "\e[1;32m"
// // #define YEL "\e[1;33m"


class Server
{
private:
    int                             _port;
    int                             _serverFd;
    std::string                     _password;
    static bool                     _signalFlag;
    std::vector<Client>             _clients;
    std::vector<struct pollfd>      _fds;
    //std::map<std::string, Channel>  _channels;
    std::vector<Channel>            _channels;

    Channel* getOrCreateChannel(const std::string& name);

    // Helpers / parser
    Client* getClient(int fd);
    Client* getClientByNick(const std::string& nick);
    void parseCommand(Client* cli, const std::string& cmd);
	void handshake(Client *cli, const std::string &cmd);
//    void tryRegister(Client& client);

    // Handlers de comandos
	void handleCap(Client *cli); //, const std::vector<std::string> &tokens);
    void handlePass(Client* cli, const std::vector<std::string>& tokens);
    void handleNick(Client* cli, const std::vector<std::string>& tokens);
    void handleUser(Client* cli, const std::vector<std::string>& tokens);
    void handleJoin(Client* cli, const std::vector<std::string>& tokens);
    void handlePrivmsg(Client* cli, const std::vector<std::string>& tokens);
    void handleTopic(Client* cli, const std::vector<std::string>& tokens);
    void handleMode(Client* cli, const std::vector<std::string>& tokens);

    void sendToClient(Client& client, const std::string& message);

public:
    Server();
    void serverInit(int port, std::string& pwd);
    void createSocket();
    void acceptNewClient();
    void receiveNewData(int fd);
    void closeFds();
    void clearClient(int fd);
  //void sendToClient(Client& client, const std::string& message);

  // Utilities
    static void signalHandler(int signum);
//	static bool isValidPasswordArg(const std::string &pass);
//	static bool isValidPortArg(const int &port);
	static std::string joinFrom(const std::vector<std::string> &v, size_t start, const std::string &sep = " ");

	
};

#endif