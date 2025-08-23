
#include <iostream>
#include "Server.hpp"


int main(int argc, char **argv) {

	int port;
	std::string pass;

	std::cout << "Proces ID: " << getpid() << std::endl;//

	try {
		//VALIDACIONES
		if (argc != 3) {
			throw(std::runtime_error("Error: Invalid argument number.\nUsage ./ircserv <port> <password>"));
		}

		port = atoi(argv[1]);
		if (port <= 1023 || port > 65535) {
			throw(std::runtime_error("Error: Recommended port for a non encrypted IRC: 6667"));
		}
		
		pass = argv[2];
		if (!Server::isValidPasswordArg(pass)) {
			throw(std::runtime_error("Error: Password must contain only printable ASCII chars (no spaces) and max 20 chars"));
		}
		//CREACION Y EJECUCION DEL SERVIDOR
		Server irc(port, pass);
		irc.run(); // bucle infinito

		std::cout << "Server terminated" << std::endl;
		return 0; // aunque nunca llegara aqui, lo ponemos por completar el flujo y claridad
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
