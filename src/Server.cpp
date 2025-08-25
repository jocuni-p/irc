
#include "Server.hpp"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstdio> // perror
#include <cerrno>

// METODO STATIC fuera de la clase (no tengo claro si ponerlo aqui suelto)???????
static void setNonBlockingOrExit(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl"); // OJO: revisar si lo dejo asi o mejor un throw ?????
        close(fd);
        exit(1); // No tengo claro si salir asi o con EXIT_FAILURE, cerrar todo limpiamente (fds)
    }
}


Server::Server(const int port, const std::string& password)
: _port(port), _password(password), _server_fd(-1) { // server_fd -1 por seguridad y robustez
    initSocket(); 
}

// DESTRUCTOR
Server::~Server() {
    if (_server_fd != -1)
        close(_server_fd);
}

// INICIALIZA EL SOCKET DEL SERVER, LO PONE EN ESCUCHA Y LO ANAYDE AL VECTOR _fds
void Server::initSocket() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1) {
        throw std::runtime_error("Error at socket creation"); // Gestionar salida limpia del prog
    }
    setNonBlockingOrExit(_server_fd);

	// LIBERA PUERTO RAPIDAMENTE para que lo pueda usar otro proceso
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == -1) {
        throw std::runtime_error("Error on setsockopt");
    }

	// CONFIGURA STRUCT PARA EL SOCKET (asigna IP y PORT)
    std::memset(&_server_addr, 0, sizeof(_server_addr));
    _server_addr.sin_family = AF_INET; // tipo de domain/IP (ipv4)
    _server_addr.sin_port = htons(_port); // puerto de entrada (donde escucha)
    _server_addr.sin_addr.s_addr = INADDR_ANY; // rango de IPs que escuchará (ANY: todas)

	// ENLAZA SERVER SOCKET(fd) a su struct de datos(socket <-> IP + PORT)
    if (bind(_server_fd, (struct sockaddr*)&_server_addr, sizeof(_server_addr)) == -1) {
        throw std::runtime_error("Error on bind");
    }

	// PONE EN ESCUCHA EL PUERTO del socket (fd, num conexiones admitidas en cola)
    if (listen(_server_fd, 5) == -1) {
        throw std::runtime_error("Error on listen");
    }

	// AÑADO struct pollfd DEL SERVER AL VECTOR _fds, será el elemento [0]
    struct pollfd server_pollfd;
    server_pollfd.fd = _server_fd; // fd a monitorear
    server_pollfd.events = POLLIN; // mascara de bits (indica eventos que escucha)
    server_pollfd.revents = 0; // mascara de bits rellenada por poll()(con los eventos ya ocurridos)
    _fds.push_back(server_pollfd); // Anyade la struct del server al vector de pollfds

    std::cout << "Server ready on port " << _port << std::endl;
}

// MANEJA CONEXION AL SERVER DE LOS fd DESCONOCIDOS (NUEVOS CLIENTES)
void Server::handleNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(_server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept"); // OJO: quizas deberia lanzar Exception ?????
        return;
    }
    setNonBlockingOrExit(client_fd);
    std::cout << "New client in fd " << client_fd << std::endl;

	// AÑADO struct pollfd DEL NEW CLIENT AL VECTOR _fds
    struct pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    client_pollfd.revents = 0;
    _fds.push_back(client_pollfd);

	// CREA OBJ Client Y LO ANYADE AL MAP DE _clients
    _clients[client_fd] = Client(client_fd);
}

//MANEJA CONEXION AL SERVER DE LOS fd CONOCIDOS (MENSAJES)
void Server::handleClientMessage(size_t i) {
    char buffer[512];
    std::memset(buffer, 0, 512);
    int fd = _fds[i].fd;
    int bytes = recv(fd, buffer, 511, 0); // el byte 512 es para el cierre \0, el 0 es una flag

    if (bytes <= 0) { // cliente cerro conexion o hubo error de lectura
        std::cout << "Cliente desconectado fd=" << fd << std::endl;
        close(fd);
        _clients.erase(fd);
        _fds.erase(_fds.begin() + i); // borramos el fb desconectado del vector -fds
        return;
    }

    std::string msg(buffer);
    std::cout << "Mensaje de " << fd << ": " << msg;

    send(fd, "Servidor: mensaje recibido\r\n", 29, 0); // 29: es num de bytes que envia
}

// ESCUCHA PERMANENTE DE ACTIVIDAD CON poll()
void Server::run() {
    while (true) { // para manejar las senyales podemos poner com arg del bucle el flag atomico global
        int activity = poll(&_fds[0], _fds.size(), -1); // timeout: -1 = espera indefinida (bloqueante)
        if (activity < 0) throw std::runtime_error("Error en poll");

        for (size_t i = 0; i < _fds.size(); ++i) {
            if (_fds[i].revents & POLLIN) {
                if (_fds[i].fd == _server_fd) { // actividad en el fd del server
                    handleNewConnection();
                } else { // actividad en algun fd de cliente
                    handleClientMessage(i);
                }
            }
        }
    }
}
