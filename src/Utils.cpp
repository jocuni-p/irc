#include "../include/Utils.hpp"


bool Utils::isValidPasswordArg(const std::string &password) {
	if (password.empty() || password.size() > 20)
		return (false);
	for (size_t i = 0; i < password.size(); i++) {
		if (password[i] < 33 || password[i] > 126) { // Permitir todos los caracteres ASCII imprimibles (33-126) excepto espacios
			return (false);
		}
	}
	return (true);
}

bool Utils::isValidPortArg(const int &port) {
	if (port <= 1023 || port > 65535) {
		return (false);
	}
	return (true);
}

// Helper para dividir un string en tokens
std::vector<std::string> Utils::split(const std::string& str, char delim)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim))
        tokens.push_back(token);
    return tokens;
}