#include <iostream>
#include "debug.hpp"


//Muestra los canales que hay y que clientes hay en cada uno
void printChannels(const std::vector<Channel>& _channels)
{
    std::cout << "---- Lista de canales ----" << std::endl;
    for (size_t i = 0; i < _channels.size(); ++i)
    {
        const Channel &ch =_channels[i];
        std::cout << "[" << i << "] Nombre: " << ch.getName();

        if (ch.hasTopic())
            std::cout << " | Topic: " << ch.getTopic();

        std::cout << "\n    Clientes (" << ch.getClients().size() << "): ";

        const std::set<int> &clients = ch.getClients();
        std::set<int>::const_iterator it = clients.begin();
        for (; it != clients.end(); ++it)
        {
            std::cout << *it;
            std::set<int>::const_iterator next = it;
            ++next;
            if (next != clients.end())
                std::cout << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << "--------------------------" << std::endl;
}

// Muestra los clientes+fd autenticados y los conectados sin autenticar 
void printClients(const std::vector<Client>& _clients) {
    std::cout << "---- Lista de clientes ----" << std::endl;
    for (size_t i = 0; i < _clients.size(); ++i) {
		if (_clients[i].getStatus() == AUTHENTICATED) {
        	std::cout << "Cliente " << i << " FD=" << _clients[i].getFd() 
			<< " " << _clients[i].getNickname() << std::endl;
    	} else {
			std::cout << "Cliente " << i << " FD=" << _clients[i].getFd() 
			<< " " << _clients[i].getNickname() << " NOT_AUTHENTICATED" << std::endl;
		}
	}
}