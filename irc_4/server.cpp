#include <iostream>
#include <cstring>      // memset()
#include <cstdlib>      // exit
#include <unistd.h>     // close()
#include <sys/socket.h> // socket(), bind(), listen(), accept(), recv(), send()
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // inet_ntoa()
#include <poll.h>
#include <vector>


#define PORT 6667
#define MAX_BUFFER 1024 //Duplica lo establecido como max (512 bytes) en el protocolo IRC

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "Servidor TCP IRC - MODO SIMPLE RECONECTABLE" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << " Este programa inicia un servidor TCP en " << std::endl;
    std::cout << " el puerto 6667 y acepta 1 conexión     " << std::endl;
    std::cout << " de cliente (por ejemplo, usando netcat). " << std::endl;
    std::cout << "--------------------------------------------" << std::endl;
    std::cout << " Para probarlo desde otra terminal:       " << std::endl;
    std::cout << "     nc localhost 6667                    " << std::endl;
    std::cout << "============================================" << std::endl << std::endl;

	//CREA UN SOCKET
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // (tipo de dominio ipv4, tipo de socket:TCP)
    if (server_fd == -1) {
        std::cerr << "Error al crear el socket." << std::endl;
        return 1;
    }

	// CONFIGURA STRUCT PARA EL SOCKET (con los params que quiero para el server)
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // tipo de domain/IP (ipv4, ipv6, ...)
    server_addr.sin_port = htons(PORT); // puerto de entrada (donde escucha)
    server_addr.sin_addr.s_addr = INADDR_ANY; //rango de IPs que escuchara (ANY: todas)

	// Config para dejar libre el puerto rapidamente despues de usarlo y que lo pueda usar otro proceso
	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

	// ENLAZA el socket(fd) a la struct(IP:puerto)
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Error en bind()." << std::endl;
        close(server_fd);
        return 1;
    }

	// ESCUCHA conexiones entrantes (admitira hasta 5 en cola)
    if (listen(server_fd, 5) == -1) {
        std::cerr << "Error en listen()." << std::endl;
        close(server_fd);
        return 1;
    }

	// Declaro un vector (de structs pollfd) que lo usara poll() para gestionar lo que pasa en todos los sockets
	std::vector<struct pollfd> fds;

	//anyado el servidor con sus datos al vector
	struct pollfd server_pollfd; //declaro una struct con nombre del server
	server_pollfd.fd = server_fd; // le digo el fd del server
	server_pollfd.events = POLLIN; // le digo que eventos ha de escuchar
	fds.push_back(server_pollfd); // anyado la struct al final del vector


	// MANTIENE EL SERVER EN MARCHA ESCUCHANDO (AUNQUE SE CIERRE UN CLIENTE)
===========================================
	
	while (true) {
		int activity = poll(fds.data(), fds.size(), -1); // -1 = espera indefinida

		if (activity < 0) {
			std::cerr << "Error en poll()." << std::endl;
			break;
		}

		for (size_t i = 0; i < fds.size(); ++i) {
			if (fds[i].revents & POLLIN) {
				if (fds[i].fd == server_fd) {
					// Nueva conexión
					struct sockaddr_in client_addr;
					socklen_t client_len = sizeof(client_addr);
					int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
					if (client_fd == -1) {
						std::cerr << "Error en accept()." << std::endl;
						continue;
					}
					std::cout << "Cliente conectado desde "
							<< inet_ntoa(client_addr.sin_addr) << ":"
							<< ntohs(client_addr.sin_port) << std::endl;

					// Añadir nuevo cliente al vector de fds
					struct pollfd client_pollfd;
					client_pollfd.fd = client_fd;
					client_pollfd.events = POLLIN;
					fds.push_back(client_pollfd);
				} else {
					// Mensaje de un cliente existente
					char buffer[MAX_BUFFER];
					std::memset(buffer, 0, MAX_BUFFER);
					int bytes = recv(fds[i].fd, buffer, MAX_BUFFER - 1, 0);
					if (bytes <= 0) {
						std::cout << "Cliente desconectado (socket " << fds[i].fd << ")" << std::endl;
						close(fds[i].fd);
						fds.erase(fds.begin() + i);
						--i; // ajustar índice tras eliminar
					} else {
						std::cout << "Mensaje (" << fds[i].fd << "): " << buffer;
						const char* respuesta = "Servidor: mensaje recibido\n";
						send(fds[i].fd, respuesta, std::strlen(respuesta), 0);
					}
				}
			}
		}
	}









	===================================
	while (true) {
        std::cout << "Esperando conexión...\n" << std::endl;

		// ACEPTA UNA CONEXION y devuelve su nuevo socket con su struct (IP y port)
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            std::cerr << "Error en accept()." << std::endl;
            continue; // Si hay error no se cierra y sigue en escucha
        }

		//DEBUGG INFO SOBRE EL CLIENTE ACEPTADO (LOCAL)-------------------
		std::cout << "--------------------------" << std::endl;
		std::cout << "Socket asignado al cliente: " << client_fd << std::endl;
		std::cout << "Cliente conectado desde "
                  << inet_ntoa(client_addr.sin_addr) << ":"
                  << ntohs(client_addr.sin_port) << std::endl;

		struct sockaddr_in local_addr;
		socklen_t local_len = sizeof(local_addr);
		//obtiene la direccion local asociada al cliente y la guarda en local_addr
		// no es obligatorio, pero es util para mostrar info del lado del servidor (en que IP y puerto ha aceptado la conexion)
		if (getsockname(client_fd, (struct sockaddr*)&local_addr, &local_len) == -1) {
			std::cerr << "Error al obtener dirección local." << std::endl;
		} else {
			//muestra por consola desde que IP y puerto local se ha aceptado la conexion con cliente
			std::cout << "Servidor conectado desde "
					<< inet_ntoa(local_addr.sin_addr)
					<< ":" << ntohs(local_addr.sin_port) << std::endl;
			std::cout << "-----------------------" << std::endl;
		}
		//-------------------------------------------------------------


		// GESTION BASICA DE MENSAJES
        char buffer[MAX_BUFFER];
        while (true) {
            std::memset(buffer, 0, MAX_BUFFER);
            int bytes = recv(client_fd, buffer, MAX_BUFFER - 1, 0);
            if (bytes == 0) {
                std::cout << "Cliente desconectado..." << std::endl;
                close(client_fd);
                break;
				
            } else if (bytes < 0) {
                std::cout << "Error al recibir datos." << std::endl;
                break;
                
            } else {
            std::cout << "Mensaje recibido: " << buffer;
            const char* respuesta = "Servidor: mensaje recibido\n";
            send(client_fd, respuesta, std::strlen(respuesta), 0);
            }
        }
    }

    close(server_fd); // Teoricamente nunca se alcanzará este punto
    return 0;
}
