#include <iostream>
#include <cstring>      // std::memset(), std::strlen()
#include <cstdlib>      // exit
#include <unistd.h>     // close()
#include <sys/socket.h> // socket(), bind(), listen(), accept(), recv(), send()
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_ntoa()
#include <poll.h>
#include <vector>
#include <fcntl.h>      // fcntl()
#include <cerrno>       // errno
#include <cstdio>       // perror()

#define PORT 6667
#define MAX_BUFFER 512 // Establecido como max bytes en el protocolo IRC

// Pequeña función utilitaria para poner un fd en non-blocking
// El subject pide explícitamente: fcntl(fd, F_SETFL, O_NONBLOCK);
void setNonBlockingOrExit(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL, O_NONBLOCK)");
        close(fd);
        exit(EXIT_FAILURE);
    }
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "Servidor TCP IRC - MULTIPLES CLIENTES     " << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << " Este programa inicia un servidor TCP en  " << std::endl;
    std::cout << " puerto 6667 y acepta multiples conexiones" << std::endl;
    std::cout << " de cliente (por ejemplo, usando netcat). " << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << " Para probarlo desde otra terminal:       " << std::endl;
    std::cout << "     nc localhost 6667                    " << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;

    // CREA UN SOCKET
	int server_fd = socket(AF_INET, SOCK_STREAM, 0); // (familia de protocolo de dominio ipv4, tipo de socket paraTCP, protocolo (si 0, el sistema lo elige))
	if (server_fd == -1) {
        std::cerr << "Error al crear el socket." << std::endl;
        return 1;
    }

    // Poner el socket de escucha en non-blocking (requisito del subject)
    setNonBlockingOrExit(server_fd);

    // CONFIGURA STRUCT PARA EL SOCKET (asigna IP y PORT)
    struct sockaddr_in server_addr; // Declaro la struct propia de un socket
    std::memset(&server_addr, 0, sizeof(server_addr)); // inicializar a 0 por seguridad
    server_addr.sin_family = AF_INET; // tipo de domain/IP (ipv4, ipv6, ...)
    server_addr.sin_port = htons(PORT); // puerto de entrada (donde escucha)
    server_addr.sin_addr.s_addr = INADDR_ANY; // rango de IPs que escuchará (ANY: todas)

    // Config para LIBERAR PUERTO RAPIDAMENTE después de usarlo y que lo pueda usar otro proceso
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == -1) {
        perror("setsockopt(SO_REUSEADDR)");
        close(server_fd);
        return 1;
    }

    // ENLAZA el socket(fd) recien creado a la struct de datos(IP, PORT, ...)
	// Necesita el tamanyo(len en bytes) de la struct para saber a que familia de protocolo 
	// pertenece (ipv4, ipv6,..) y asi el kernel solo copia los bytes necesarios 
	// y no sale de ese espacio de la memoria asignada
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Error en bind()." << std::endl;
        close(server_fd);
        return 1;
    }

    // PONE EN ESCUCHA EL PUERTO del socket del server a conexiones entrantes (fd, num conexiones que admite en cola esperando)
    if (listen(server_fd, 5) == -1) {
        std::cerr << "Error en listen()." << std::endl;
        close(server_fd);
        return 1;
    }

    // VECTOR DE FDS (structs pollfd): lo usará poll() para gestionar todo lo que pase en los sockets
    std::vector<struct pollfd> fds;

    // Declaro una struct para el server y seteo sus parámetros
    // AÑADO EL SERVER AL VECTOR, será el primero [0] en el vector
    struct pollfd server_pollfd;
    server_pollfd.fd = server_fd;
    server_pollfd.events = POLLIN;
    server_pollfd.revents = 0;
    fds.push_back(server_pollfd); // añado la struct al final del vector (que es el [0], porque es el primero)


    /** SERVER ESCUCHANDO INDEFINIDAMENTE pero poll() lo mantiene en espera, "dormido",
     * sin consumir CPU, si no hay actividad. Si hay actividad "lo despierta", maneja
     * el evento rápidamente y vuelve a ponerlo en espera.
     */
    while (true) {

        std::cout << "Esperando actividad...\n" << std::endl;

        // LLamo a poll(vector de fds) para ver si ha detectado actividad (en el POLLIN de algún fd).
        // Si la actividad esta en el server_fd: es un cliente intentando conectarse.
        // Si la actividad esta en un client_fd: es un cliente conectado enviando un mensaje o desconectandose
        // El .data convierte el vector en un array tipo C, tal y como poll() lo necesita
        // timeout = -1 -> espera indefinidamente hasta que haya actividad. Aquí nuestro poll() es bloqueante,
        // para ahorrar recursos, aunque nuestros sockets serán no bloqueantes, como pide el subject.
        // Retorno de poll(). >0: num de fds con eventos, 0: timeout (sin eventos), -1: error
		// DORMIDO AQUI HASTA QUE HAYA ALGUNA ACTIVIDAD
		int activity = poll(fds.data(), fds.size(), -1);

        // si poll() retorna fallo (-1), salimos del bucle y se cierra el programa (OJO: puede que haya que limpiar memorias o buffers)
        if (activity < 0) {
            std::cerr << "Error en poll()." << std::endl;
            break;
        }

        // Recorre TODOS los elementos del vector para ver quién es el que ha tenido actividad
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

                    // Ponemos el cliente en non-blocking tal y como exige el subject
                    setNonBlockingOrExit(client_fd);

                    std::cout << "Nuevo Cliente en socket "
                              << client_fd << " (desde "
                              << inet_ntoa(client_addr.sin_addr) << ":"
                              << ntohs(client_addr.sin_port)
                              << ")" << std::endl;

                    // Añadimos el nuevo cliente al vector<pollfd> fds
                    struct pollfd client_pollfd; // Declaro su struct
                    client_pollfd.fd = client_fd; // Seteo su fd
                    client_pollfd.events = POLLIN; // Seteo sus eventos de escucha
                    client_pollfd.revents = 0; // Inicializo su lista de eventos recividos
                    fds.push_back(client_pollfd); // Anyado a la cola del vector

                } else { // Si la actividad que se ha dado en el socket es de un cliente
                    // Preparo un buffer para alojar el mensaje y lo recibo
                    char buffer[MAX_BUFFER];
                    std::memset(buffer, 0, MAX_BUFFER);
                    int bytes = recv(fds[i].fd, buffer, MAX_BUFFER - 1, 0); // El -1 es para el '\0' de cierre del string. El 0 es un flag.

                    // Si el cliente ha cerrado la conexión
                    if (bytes == 0) {
                        std::cout << "Cliente ha cerrado la conexion (socket " << fds[i].fd << ")" << std::endl;
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i; // ajustar índice tras eliminar

                    // Si bytes < 0: error en lectura
                    } else if (bytes < 0) {
                        std::cout << "Error en la lectura del cliente (socket " << fds[i].fd << ") errno=" << errno << std::endl;
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                        std::cout << "He cerrado y borrado el socket" << std::endl;

                    // Si el mensaje es válido: lo muestro por consola y envío un acuse de recibo al cliente
                    } else {
                        std::cout << "Mensaje del socket (" << fds[i].fd << "): " << buffer;
                        const char* respuesta = "Servidor: mensaje recibido\r\n";
                        send(fds[i].fd, respuesta, std::strlen(respuesta), 0);
                    }
                }
            }
        }
    }

    close(server_fd); // Solo se alcanzará este punto si falla poll()
    return 0;
}
