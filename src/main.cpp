
#include <iostream>
#include "Server.hpp"


int main(int argc, char **argv) {

	int port;
	std::string pass;

	try {
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

	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1; // No estoy seguro de este numero de error
	}
	return 0;
}
