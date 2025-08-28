
#include "Client.hpp"

// CONSTRUCTOR
//Client::Client(int fd) : _fd(fd) {}
Client::Client() : _fd(-1), _status(NOT_AUTHENTICATED) {}
Client::Client(int fd) : _fd(fd), _status(NOT_AUTHENTICATED) {}

//DESTRUCTOR
Client::~Client() {
	if (_fd != -1)
		close(_fd);
}

// --- Getters / Setters básicos ---
int Client::getFd() const { return _fd; }
void Client::setFd(int fd) { _fd = fd; }

const std::string& Client::getIp() const { return _ip; }
void Client::setIp(const std::string& ip) { _ip = ip; }

const std::string& Client::getNick() const { return _nickname; }
void Client::setNick(const std::string& nick) { _nickname = nick; }

const std::string& Client::getUser() const { return _username; }
void Client::setUser(const std::string& user) { _username = user; }

LoginStatus Client::getStatus() const { return _status; }
void Client::setStatus(LoginStatus s) { _status = s; }


// --- Buffer de entrada ---
void Client::appendToBuffer(const std::string& data) {
	_buffer += data;
	// Si quisieras protegerte de flood:
	if (_buffer.size() > 4096) {  // ejemplo: límite de 8 comandos (~512*8)
		_buffer.clear(); // o desconectar cliente
	}
}

std::string& Client::getBuffer() { return _buffer; }
void Client::clearBuffer() { _buffer.clear(); }















// int Client::getFd() const { return _fd; }

// std::string Client::getNickname() const { return _nickname; }

// void Client::setNickname(const std::string &nick) { _nickname = nick; }

// void Client::appendBuffer(const std::string &data) {
// 	_buffer += data;
// }

// std::string Client::extractMessage() {
// 	//obtiene la linea de _buffer
// }