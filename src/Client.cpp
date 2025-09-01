#include "Client.hpp"

Client::Client() : _fd(-1), _passAccepted(false), _authenticated(false) {}
Client::~Client() {}

// --- Getters ---
int Client::getFd() const { return _fd; }
std::string Client::getIp() const { return _ip; }
std::string Client::getNickname() const { return _nickname; }
std::string Client::getUsername() const { return _username; }
std::string Client::getRealname() const { return _realname; }
bool Client::isAuthenticated() const { return _authenticated; }
std::string &Client::getBuffer() { return _buffer; }

// --- Setters ---
void Client::setFd(int newFd) { _fd = newFd; }
void Client::setIp(const std::string &newIp) { _ip = newIp; }
void Client::setNickname(const std::string &nick) { _nickname = nick; }
void Client::setUsername(const std::string &user) { _username = user; }
void Client::setRealname(const std::string& real) { _realname = real; }
void Client::setAuthenticated(bool status) { _authenticated = status; }

// PASS
bool Client::hasPassAccepted() const { return _passAccepted; }
void Client::setPassAccepted(bool status) { _passAccepted = status; }

// --- Buffer handling ---
void Client::appendToBuffer(const std::string& data) { _buffer += data; }
void Client::clearBuffer() { _buffer.clear(); }