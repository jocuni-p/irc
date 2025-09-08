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

//Elimina '\r\n' del final de los mensajes
std::string Utils::stripCRLF(const std::string& msg) {
    if (msg.size() >= 2 && msg[msg.size() - 2] == '\r' && msg[msg.size() - 1] == '\n')
        return msg.substr(0, msg.size() - 2);
    return msg;
}

std::string Utils::joinFrom(const std::vector<std::string>& v, size_t start, const std::string &sep)
{
    std::string out;
    for (size_t i = start; i < v.size(); ++i)
    {
        if (i > start)
            out += sep;
        out += v[i];
    }
    return out;
}
/*
//esta funcion podria usar el send para enviar el mensaje y codigo de error al socket del cliente concreto que su mensaje genere el error
//los errores estan extraido de https://www.rfc-es.org/rfc/rfc1459-es.txt /seccion 6 Respuestas
//el parametro arg es un puntero para poder pasar NULL cuando es un tipo de error que no requiere de ese valor
void	Server::handlerError(ErrorCode code, Client &client, std::string arg)
{
	std::string msg = ":" + _server_name + " ";
	std::ostringstream cast;
	cast << code;
	msg += cast.str();
	msg += " " + client.getNick() + " ";
	switch (code) 
	{
		case ERR_NOSUCHNICK://401
			msg += arg + " :User not found"; break;
		case ERR_NOSUCHCHANNEL://403
			msg += arg + " :No such channel"; break;
		case ERR_CANNOTSENDTOCHAN:
			msg += arg + " :Cannot send to channel"; break;
		case ERR_TOOMANYCHANNELS://405
			msg += arg + " :You have joined too many channels"; break;
		case ERR_TOOMANYTARGETS://407
			msg += arg + " :Too many targets"; break;
		case ERR_NORECIPIENT://411
			msg += ":No recipient given " + arg; break;
		case ERR_NOTEXTTOSEND://412
			msg += ":No text to send"; break;
		case ERR_UNKNOWNCOMMAND://421
			msg += arg + " :Unknown command"; break;
		case ERR_NONICKNAMEGIVEN://431
			msg += ":No nickname given"; break;
		case ERR_ERRONEUSNICKNAME://432
			msg += arg + " :Erroneous nickname"; break;
		case ERR_NICKNAMEINUSE://433
			msg += arg + " :Nickname is already in use"; break;
		case ERR_USERNOTINCHANNEL://441
			msg += arg + " :They aren't on that channel"; break;
		case ERR_NOTONCHANNEL://442
			msg += arg + " :You're not on that channel"; break;
		case ERR_USERONCHANNEL://443
			msg += arg + " :is already on channel"; break;
		case ERR_NOTREGISTERED://451
			msg += ":You have not registered"; break;
		case ERR_NEEDMOREPARAMS://461
			msg += arg + " :Not enough parameters"; break;
        case ERR_ALREADYREGISTRED://462
			msg += ":Unauthorized command (already registered)"; break;
		case ERR_PASSWDMISMATCH://464
			msg += ":Password incorrect"; break;
		case ERR_KEYSET://467
			msg += arg + " :Channel key already set"; break;
		case ERR_CHANNELISFULL: //471
			msg += arg + " :Cannot join channel (channel is full)"; break;
		case ERR_UNKNOWNMODE://472
			msg += arg + " :is unknown mode char to me"; break;
		case ERR_INVITEONLYCHAN: //473
			msg += arg + " :Cannot join channel (invitation required)"; break;
		case ERR_BADCHANNELKEY://475
			msg += arg + " :Cannot join channel (incorrect passkey)"; break;
		case ERR_BADCHANMASK://476
			msg += arg + " :Channel names must start with '#' and be 32 characters max"; break;
		case ERR_CHANOPRIVSNEEDED://482
			msg += arg + " :You're not channel operator"; break;
		case ERR_INVALIDKEY://525
			msg += arg + " :Key is not well-formed (forbidden characters(!@#$%&*()+=;:'\"<>,.?/~) or longer than 16 chars)"; break;
		case ERR_INVALIDMODEPARAM://696
			msg += arg; break;
        default:
            //std::cout << "Error desconocido." << std::endl;
            break;
    }
	msg += "\r\n";

	if (send(client.getSocket(), msg.c_str(), msg.size(), 0) != (ssize_t)msg.size())
		throw std::runtime_error("handlerError: send()");
	std::cout << "<" << client.getSocket() << "> " << GREEN << ">> " << RESET << msg;//debug
}
	*/

