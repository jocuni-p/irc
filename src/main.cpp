
#include <iostream>
#include <csignal> // sig_atomic_t
#include "Server.hpp"

volatile sig_atomic_t g_running = 1; //Var global aceptado por c++98 para manejo de las senyales (controla el bucle de run)


static void signalHandler(int signum) { // recogera realmente la senyal????
    g_running = 0; // Fuerza la salida del loop del server
}


int main(int argc, char **argv) {

	int port;
	std::string pass;


	try {
		//VALIDACIONES
		if (argc != 3) {
			throw(std::runtime_error("Error: Invalid argument number.\nUsage ./ircserv <port> <password>"));
		}

		port = atoi(argv[1]);
		if (!Server::isValidPortArg(port)) {
			throw(std::runtime_error("Error: Recommended port for a non encrypted IRC: 6667"));
		}
		
		pass = argv[2];
		if (!Server::isValidPasswordArg(pass)) {
			throw(std::runtime_error("Error: Password must contain only printable ASCII chars (no spaces) and max 20 chars"));
		}
		//CREACION Y EJECUCION DEL SERVIDOR
		Server irc(port, pass);

		// Captura señales
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);


		irc.run(); // bucle infinito


		std::cout << "Server shutdown gracefully" << std::endl;
		// Se habra llamado al destructor y este al shutdown ??
		return 0; //Deberia salir por aqui si se hace un Ctrl+C y se maneja adecuadamente la senyal
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
