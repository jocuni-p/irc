
#include "Client.hpp"

// CONSTRUCTOR
Client::Client(int fd) : _fd(fd) {}

//DESTRUCTOR
Client::~Client() {
	if (_fd != -1)
		close(_fd);
}

int Client::getFd() const { return _fd; }

std::string Client::getNickname() const { return _nickname; }

void Client::setNickname(const std::string &nick) { _nickname = nick; }

void Client::appendBuffer(const std::string &data) {
	_buffer += data;
}

std::string Client::extractMessage() {
	//obtiene la linea de ???
}