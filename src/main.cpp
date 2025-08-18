
#include <iostream>
#include "Server.hpp"

int main(int argc, char **argv) {

	int port;
	std::string pass;

	try {
		if (argc != 3) {
			throw(std::runtime_error("Invalid argument number."));
			return (0);
		}
		port = atoi(argv[1]);
		if (port <= 1023 || port > 65535) {
			throw(std::runtime_error("Recommended port for a non encrypted IRC: 6667"));
		}
		pass = argv[2];
		if (pass.empty()) {
			throw(std::runtime_error("Argument <password> not informed."));
		}

	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return -1;
	}
	return 0;
}
