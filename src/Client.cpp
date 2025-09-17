#include "Client.hpp"

Client::Client() : _fd(-1), _status(NOT_AUTHENTICATED), _toBeRemoved(false) {}
Client::~Client() {}

// --- Getters ---
int Client::getFd() const { return _fd; }
std::string Client::getIp() const { return _ip; }
std::string Client::getNickname() const { return _nickname; }
std::string Client::getUsername() const { return _username; }
std::string Client::getRealname() const { return _realname; }
LoginStatus Client::getStatus() const { return _status; }
std::string &Client::getBuffer() { return _buffer; }

// --- Setters ---
void Client::setFd(int newFd) { _fd = newFd; }
void Client::setIp(const std::string &newIp) { _ip = newIp; }
void Client::setNickname(const std::string &nick) { _nickname = nick; }
void Client::setUsername(const std::string &user) { _username = user; }
void Client::setRealname(const std::string& real) { _realname = real; }
void Client::setStatus(LoginStatus s) { _status = s; }

// Removed fd's handling
void Client::markForRemoval() { _toBeRemoved = true; }
bool Client::isMarkedForRemoval() const { return _toBeRemoved; }

// --- Buffer handling ---
void Client::appendToBuffer(const std::string& data) { _buffer += data; }
void Client::clearBuffer() { _buffer.clear(); }