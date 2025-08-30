
#include <iostream>
#include <csignal> // sig_atomic_t
#include "../include/Server.hpp"

//volatile sig_atomic_t g_running = 1; //Var global aceptado por c++98 para manejo de las senyales (controla el bucle de run)


// static void signalHandler(int signum) { // recogera realmente la senyal????
//     g_running = 0; // Fuerza la salida del loop del server
// }


int main(int argc, char **argv) {

	int port;
	std::string pass;

	std::cout << "Proces ID: " << getpid() << std::endl;

	try {
		//VALIDACIONES
		if (argc != 3) {
			throw(std::runtime_error("Failure.\nUsage ./ircserv <port> <password>"));
		}

		port = atoi(argv[1]);
		if (!Server::isValidPortArg(port)) {
			throw(std::runtime_error("Failure: Recommended port for a non encrypted IRC: 6667"));
		}
		
		pass = argv[2];
		if (!Server::isValidPasswordArg(pass)) {
			throw(std::runtime_error("Failure: Password must contain only printable ASCII chars (no spaces) and max 20 chars"));
		}
		//CREACION Y EJECUCION DEL SERVIDOR
		Server irc(port, pass);

		// Captura de señales
        std::signal(SIGINT, Server::signalHandler);
        std::signal(SIGTERM, Server::signalHandler);
//		std::signal(SIGQUIT, signalHandler); // no necesario manejar, es mas para debbuger (termina y genera un core dump para poder debbugar los errores)

		irc.run(); // bucle de aceptacion conexiones

//		std::cout << " Server shutdown gracefully" << std::endl;
		return 0; //Deberia salir por aqui si se hace un Ctrl+C y se maneja adecuadamente la senyal
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
