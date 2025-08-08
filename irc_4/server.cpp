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
    std::cout << "Servidor TCP IRC - MULTIPLES CLIENTES" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << " Este programa inicia un servidor TCP en " << std::endl;
    std::cout << " el puerto 6667 y acepta multiples conexiones" << std::endl;
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

	// CONFIGURA STRUCT PARA EL SOCKET (asigna IP y PORT)
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // tipo de domain/IP (ipv4, ipv6, ...)
    server_addr.sin_port = htons(PORT); // puerto de entrada (donde escucha)
    server_addr.sin_addr.s_addr = INADDR_ANY; //rango de IPs que escuchara (ANY: todas)

	// Config para LIBERAR PUERTO RAPIDAMENTE despues de usarlo y que lo pueda usar otro proceso
	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

	// ENLAZA el socket(fd) recien creado a la struct(IP, PORT, ...)
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Error en bind()." << std::endl;
        close(server_fd);
        return 1;
    }

	// ESCUCHA conexiones entrantes (fd, los que admite en cola)
    if (listen(server_fd, 5) == -1) {
        std::cerr << "Error en listen()." << std::endl;
        close(server_fd);
        return 1;
    }

	// VECTOR DE FDS (structs pollfd): lo usara poll() para gestionar todo lo que pase en los sockets
	std::vector<struct pollfd> fds;

	// Declaro una struct para el server y seteo sus parametros
	// ANYADO EL SERVER AL VECTOR, sera el primero [0] en el vector
	struct pollfd server_pollfd; //declaro una struct con nombre del server
	server_pollfd.fd = server_fd; // le digo el fd del server
	server_pollfd.events = POLLIN; // le digo que eventos ha de escuchar
	fds.push_back(server_pollfd); // anyado la struct al final del vector (que es el [0], porque es el primero)


	/** SERVER ESCUCHANDO INDEFINIDAMENTE pero poll() lo mantiene en espera, "dormido", 
	 * sin consumir CPU, si no hay actividad. Si hay actividad "lo despierta", maneja 
	 * el evento rapidamente y vuelve a ponerlo en espera.
	 */
	while (true) {

		std::cout << "Esperando actividad...\n" << std::endl;

		// LLamo a poll(vector de fds) para ver si ha detectado actividad (en el POLLIN de algun fd).
		// Si la actividad esta en el server_fd: es un cliente intentando conectarse.
		// Si la actividad esta en un client_fd: es un cliente conectado enviando un mensaje o desconectandose
		// El .data convierte el vector en un array tipo C, tal y como poll() lo necesita
		// timeout = -1 -> espera indefinidamente hasta que haya actividad. Aqui nuestro poll() es bloqueante,
		// para ahorrar recursos, aunque nuestros sockets seran no bloqueantes, como pide el subject.
		// Retorno de poll(). >0: num de fds con eventos, 0: timeout (sin eventos), -1: error
		// activity nos dira en cuantos sockets hay actividad pendiente 
		int activity = poll(fds.data(), fds.size(), -1);

		// si poll() retorna (-1) fallo, salimos del bucle y se cierra el programa (OJO: puede que haya que limpiar memorias o buffers)
		if (activity < 0) {
			std::cerr << "Error en poll()." << std::endl;
			break;
		}

		//Recorre TODOS los elementos del vector para ver quien es el que ha tenido actividad
		for (size_t i = 0; i < fds.size(); ++i) {
			if (fds[i].revents & POLLIN) { // Si hubo un evento de POLLIN en este socket (bitwise AND)
				if (fds[i].fd == server_fd) { // Si el socket es el del server
					// Creamos un socket para el nuevo cliente (sockaddr_in, accept())
					struct sockaddr_in client_addr;
					socklen_t client_len = sizeof(client_addr);
					int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
					if (client_fd == -1) {
						std::cerr << "Error en accept()." << std::endl;
						continue;
					}
					std::cout << "Nuevo Cliente en socket " 
							<< client_fd << " (desde "
							<< inet_ntoa(client_addr.sin_addr) << ":"
							<< ntohs(client_addr.sin_port) 
							<< ")" << std::endl;

					// Añadimos el nuevo cliente al vector<pollfd> fds
					struct pollfd client_pollfd;
					client_pollfd.fd = client_fd;
					client_pollfd.events = POLLIN;
					fds.push_back(client_pollfd);
				} else { // Si la actividad se ha dado en el socket de un cliente
					// Preparo un buffer para alojar el mensaje y lo recibo
					char buffer[MAX_BUFFER];
					std::memset(buffer, 0, MAX_BUFFER);
					int bytes = recv(fds[i].fd, buffer, MAX_BUFFER - 1, 0); // El -1 es para el \0 de cierre del string. El 0 es un flag.
					// Si el mensaje es 0
					if (bytes == 0) {
						std::cout << "Cliente ha cerrado la conexion (socket " << fds[i].fd << ")" << std::endl;
						close(fds[i].fd);
						fds.erase(fds.begin() + i);
						--i; // ajustar índice tras eliminar
					// Si el mensaje es un error
					} else if (bytes < 0) {
						std::cout << "Error en la lectura del cliente (socket " << fds[i].fd << ")" << std::endl;
						close(fds[i].fd);
						fds.erase(fds.begin() + i);
						--i;
						std::cout << "He cerrado y borrado el socket" << std::endl;
					}
					// Si el mensaje es valido: lo muestro por consola y envio un acuse de recibo al cliente
					else
					{
						std::cout << "Mensaje del socket (" << fds[i].fd << "): " << buffer;
						const char* respuesta = "Servidor: mensaje recibido\n";
						send(fds[i].fd, respuesta, std::strlen(respuesta), 0);
					}
				}
			}
		}
	}

    close(server_fd); // Teoricamente nunca se alcanzará este punto
    return 0;
}

/**PROXIMO PASO: HACERLO UN SERVIDOR IRC FUNCIONAL Y QUE ACEPTE EL PROTOCOLO
 *  IRC PARA EL CLIENTE HEXCHAT. QUE RETORNE LAS RESPUESTAS ESPECIFICAS QUE 
 * ESPERA HEXCHAT PARA CONECTARSE Y QUE PUEDA GESTIONAR LOS COMANDOS PROTOCOLIZADOS DE 
 * COMUNICACION IRC.
 * HACER UN MAKEFILE.
 * QUE RECIBA LOS PARAMETROS AL EJECUTAR: ./ircserv <port> <password>
 * ATENCION: USAR LAS EXCEPCIONES PARA LA GESTION DE ERRORES, try_catch en el main.
*/
