#include <iostream>
#include <cstring>      // memset(), strlen()
#include <unistd.h>     // close()
#include <sys/socket.h> // socket(), bind(), listen(), accept(), recv(), send()
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_ntoa()

#define PORT 6667
#define MAX_BUFFER 1024

int main() {

	std::cout << "=========================================" << std::endl;
	std::cout << "       Servidor TCP IRC - MODO SIMPLE    " << std::endl;
	std::cout << "-----------------------------------------" << std::endl;
	std::cout << " Este programa inicia un servidor TCP en " << std::endl;
	std::cout << " el puerto 6667 y acepta 1 unica conexión" << std::endl;
	std::cout << " de cliente (por ejemplo, usando netcat). " << std::endl;
	std::cout << "-----------------------------------------" << std::endl;
	std::cout << " Para probarlo desde otra terminal:       " << std::endl;
	std::cout << "     nc localhost 6667                    " << std::endl;
	std::cout << "=========================================" << std::endl << std::endl;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // (tipo de dominio ipv4, tipo de socket:TCP)
    if (server_fd == -1) {
        std::cerr << "Error al crear el socket." << std::endl;
        return 1;
    }

    // Configuro la struct del sistema "sockaddr_in" con los parametros que quiero para mi server
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // define el tipo de domain/IP (ipv4, ipv6, ...)
    server_addr.sin_port = htons(PORT); // define puerto de entrada (escucha)
    server_addr.sin_addr.s_addr = INADDR_ANY; //define que IPs escuchara (ANY: todas)

    // Amarro/enlazo el socket(fd) a la struct(IP:puerto)
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Error en bind()." << std::endl;
        close(server_fd);
        return 1;
    }

    // Lo pongo a escuchar conexiones entrantes
    if (listen(server_fd, 1) == -1) {
        std::cerr << "Error en listen()." << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "Servidor escuchando en el puerto " << PORT << std::endl;

    // Acepta una conexión y devuelve un nuevo socket
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        std::cerr << "Error en accept()." << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "Cliente conectado desde "
              << inet_ntoa(client_addr.sin_addr) << ":"
              << ntohs(client_addr.sin_port) 
			  << " (socket: " << client_fd << ")"
			  << std::endl;

    // Comunicación con el cliente
    char buffer[MAX_BUFFER];
    while (true) {
        std::memset(buffer, 0, MAX_BUFFER);
        int bytes = recv(client_fd, buffer, MAX_BUFFER - 1, 0);
        if (bytes <= 0) {
            std::cout << "Cliente desconectado." << std::endl;
            break;
        }

        std::cout << "Mensaje recibido: " << buffer;

        // Respuesta fija
        const char* respuesta = "Servidor: mensaje recibido\n";
        send(client_fd, respuesta, std::strlen(respuesta), 0);
    }

    close(client_fd);
    close(server_fd);
    return 0;
}

