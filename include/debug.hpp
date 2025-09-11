// debug.hpp

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <vector>
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

void printChannels(const std::vector<Channel>& _channels);
void printClients(const std::vector<Client>& _clients);

#endif
