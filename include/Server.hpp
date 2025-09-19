#ifndef SERVER_HPP
#define SERVER_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cctype> 
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
#include "debug.hpp"


class Server
{
private:
    int                      	_port;
    int                         _serverFd;
    std::string                 _password;
    static bool                 _signalFlag;
    std::vector<Client>         _clients;
    std::vector<struct pollfd>	_fds;
    std::vector<Channel>        _channels;

    // Helpers / parser
    Client* 					getClient(int fd);
    Client* 					getClientByNick(const std::string& nick);
	Channel*					getChannelByName(const std::string &name);
    Channel* 					getOrCreateChannel(const std::string& name);
    Channel* 					findChannel(const std::string& channelName);
	void 						parseCommand(Client *cli, const std::string &cmd);
	void 						handshake(Client *cli, const std::string &cmd);

    // AUTHENTICATION COMMANDS
	void 						handleCap(Client *cli);
    void 						handlePass(Client* cli, const std::vector<std::string>& tokens);
    void 						handleNick(Client* cli, const std::vector<std::string>& tokens);
    void 						handleUser(Client* cli, const std::vector<std::string>& tokens);
    
	// COMMANDS
	void 						handleJoin(Client* cli, const std::vector<std::string>& tokens);
	void 						handleWho(Client *cli, const std::vector<std::string> &tokens);
	void 						handlePrivmsg(Client *cli, const std::vector<std::string> &tokens);
	void 						handleTopic(Client* cli, const std::vector<std::string>& tokens);
    void 						handleMode(Client* cli, const std::vector<std::string>& tokens);
    void 						handleKick(Client* cli, const std::vector<std::string>& tokens);
    void 						handleInvite(Client* cli, const std::vector<std::string>& tokens);

    void 						sendToClient(Client& client, const std::string& message);
    
    bool 						checkOperator(Client *cli, Channel *chan, const std::string& target);
    void 						showChannelModes(Client *cli, Channel *chan, const std::string& target);
    void 						broadcastModeChange(Channel *chan, Client *cli,
									const std::string& target,
									const std::string& appliedModes,
									const std::vector<std::string>& appliedArgs);
	void 						applyChannelModes(Client* cli, Channel* chan,
									const std::vector<std::string>& tokens,
									const std::string& target) ;

public:
    Server();
    void 						serverInit(int port, std::string& pwd);
    void 						createSocket();
    void 						acceptNewClient();
    void 						receiveNewData(int fd);
    void 						closeFds();
    void 						clearClient(int fd);
    void 						removeEmptyChannel(Channel* chan);

  // Utilities
    static void 				signalHandler(int signum);
	static std::string 			joinFrom(const std::vector<std::string> &v, size_t start, const std::string &sep = " ");
};

#endif