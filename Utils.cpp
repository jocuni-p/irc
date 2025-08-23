#include "Server.hpp"


bool Server::isValidPasswordArg(const std::string &pass) {
	if (pass.empty() || pass.size() > 20)
		return (false);
	for (size_t i = 0; i < pass.size(); i++) {
		if (pass[i] < 33 || pass[i] > 126) { // Permitir todos los caracteres ASCII imprimibles (33-126) excepto espacios
			return (false);
		}
	}
	return (true);
}