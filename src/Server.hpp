#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <poll.h>
#include <netinet/in.h>
#include "Client.hpp"
#include "Channel.hpp"


class Server {
private:
	// Config
	const int 						_port;
	const std::string 				_password;

	// Socket
	int 							_server_fd;
    struct sockaddr_in 				_server_addr; //struct del sistema, contiene IP y puerto del socket

	// Execution
	static bool                 	_signalFlag;  // para detectar las senyales
	std::vector<struct pollfd> 		_fds;
    std::map<int, Client> 			_clients;       // fd -> Client
//    std::map<std::string, Channel> 	_channels; // nombre -> Channel


    void initSocket();
    void handleNewConnection();
    void handleClientMessage(size_t i);
	Client *getClient(int fd);
	void disconnectClient(int client_fd);
	void shutdown(); // Limpia todo correctamente

	void parseCommand(Client *cli, const std::string &cmd); // OJO AQUI
	void handlePass(Client* cli, const std::vector<std::string>& tokens);
    void handleNick(Client* cli, const std::vector<std::string>& tokens);
    void handleUser(Client* cli, const std::vector<std::string>& tokens);
    void handleJoin(Client* cli, const std::vector<std::string>& tokens);
    void handlePrivmsg(Client* cli, const std::vector<std::string>& tokens);

public:
	Server(int port, const std::string &password);
    ~Server();
	
    void run(); // bucle de poll()

	// Utilities
	static bool isValidPasswordArg(const std::string &pass);
	static bool isValidPortArg(const int &port);
	static void signalHandler(int signum);
};



#endif
