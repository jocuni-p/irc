#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include "Client.hpp"

class Channel {
private:
    std::string _name;
    std::map<int, Client*> _members; // fd -> Client*

public:
    Channel();
    Channel(const std::string& name);
    ~Channel();

    std::string getName() const;
    void addMember(Client* client);
    void removeMember(int fd);
};

#endif

