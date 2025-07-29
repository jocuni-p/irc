#include <iostream>
#include <unistd.h>             // close()
#include <cstring>              // memset()
#include <sys/socket.h>         // socket(), bind(), listen(), accept()
#include <netinet/in.h>         // sockaddr_in
#include <arpa/inet.h>          // inet_ntoa()

#define PORT 6667              // Puerto típico del IRC
#define BACKLOG 10             // Número de conexiones en cola

int main() {

    // 1. Crear socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); //Tipo de domain (IPv4) y tipo de socket (para TCP)
    if (server_fd == -1) {
        std::cerr << "Error al crear el socket." << std::endl;
        return 1;
    }
    std::cout << "Socket creado: FD = " << server_fd << std::endl;

    // 2. Configurar socket (opcional pero recomendado: reutilizar dirección)
    // int opt = 1;
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    //     std::cerr << "❌ Error en setsockopt()." << std::endl;
    //     close(server_fd);
    //     return 1;
    // }

    // 3. Definir una dirección para servidor en la struct
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));  // Limpieza
    server_addr.sin_family = AF_INET;                   // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;           // Escuchar en todas las interfaces
    server_addr.sin_port = htons(PORT);                 // Puerto en orden de red

    // 4. Enlazar el socket a dirección IP:PUERTO
    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "❌ Error en bind()." << std::endl;
        close(server_fd);
        return 1;
    }
    std::cout << "✅ Socket enlazado al puerto " << PORT << std::endl;

    // 5. Escuchar conexiones entrantes
    if (listen(server_fd, BACKLOG) == -1) {
        std::cerr << "❌ Error en listen()." << std::endl;
        close(server_fd);
        return 1;
    }
    std::cout << "✅ Escuchando conexiones..." << std::endl;

    // 6. Aceptar conexiones
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            std::cerr << "❌ Error al aceptar conexión." << std::endl;
            continue;  // Sigue esperando otros clientes
        }

        std::cout << "🟢 Nueva conexión desde "
                  << inet_ntoa(client_addr.sin_addr) << ":"
                  << ntohs(client_addr.sin_port) << std::endl;

        // 7. (En este ejemplo simple) enviar mensaje al cliente
        const char* msg = "Hola, cliente IRC!\n";
        send(client_fd, msg, strlen(msg), 0);

        // 8. Cerrar conexión con el cliente (en IRC real, lo gestionarás con poll())
        close(client_fd);
    }

    // 9. Cerrar el socket del servidor (nunca llega aquí en este ejemplo)
    close(server_fd);
    return 0;
}
