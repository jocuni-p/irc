
#include "Server.hpp"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cerrno>

static void setNonBlockingOrExit(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        close(fd);
        exit(1);
    }
}

Server::Server(int port, const std::string& password)
: _port(port), _password(password), _server_fd(-1) { // server_fd -1 por seguridad y robustez
    initSocket();
}

Server::~Server() {
    if (_server_fd != -1)
        close(_server_fd);
}

void Server::initSocket() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1) {
        throw std::runtime_error("Error creando socket");
    }
    setNonBlockingOrExit(_server_fd);

    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == -1) {
        throw std::runtime_error("Error en setsockopt");
    }

    std::memset(&_server_addr, 0, sizeof(_server_addr));
    _server_addr.sin_family = AF_INET;
    _server_addr.sin_port = htons(_port);
    _server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_server_fd, (struct sockaddr*)&_server_addr, sizeof(_server_addr)) == -1) {
        throw std::runtime_error("Error en bind");
    }

    if (listen(_server_fd, 5) == -1) {
        throw std::runtime_error("Error en listen");
    }

    struct pollfd server_pollfd;
    server_pollfd.fd = _server_fd;
    server_pollfd.events = POLLIN;
    server_pollfd.revents = 0;
    _fds.push_back(server_pollfd);

    std::cout << "Servidor iniciado en puerto " << _port << std::endl;
}

void Server::handleNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(_server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept");
        return;
    }
    setNonBlockingOrExit(client_fd);
    std::cout << "Nuevo cliente en fd " << client_fd << std::endl;

    struct pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    client_pollfd.revents = 0;
    _fds.push_back(client_pollfd);

    _clients[client_fd] = Client(client_fd);
}

void Server::handleClientMessage(size_t i) {
    char buffer[512];
    std::memset(buffer, 0, 512);
    int fd = _fds[i].fd;
    int bytes = recv(fd, buffer, 511, 0);

    if (bytes <= 0) {
        std::cout << "Cliente desconectado fd=" << fd << std::endl;
        close(fd);
        _clients.erase(fd);
        _fds.erase(_fds.begin() + i);
        return;
    }

    std::string msg(buffer);
    std::cout << "Mensaje de " << fd << ": " << msg;

    send(fd, "Servidor: mensaje recibido\r\n", 29, 0);
}

void Server::run() {
    while (true) {
        int activity = poll(&_fds[0], _fds.size(), -1);
        if (activity < 0) throw std::runtime_error("Error en poll");

        for (size_t i = 0; i < _fds.size(); ++i) {
            if (_fds[i].revents & POLLIN) {
                if (_fds[i].fd == _server_fd) {
                    handleNewConnection();
                } else {
                    handleClientMessage(i);
                }
            }
        }
    }
}
