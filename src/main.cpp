#include "../include/Server.hpp"
#include "../include/Utils.hpp"

int main(int argc, char **argv)
{
    signal(SIGINT, Server::signalHandler);
    signal(SIGQUIT, Server::signalHandler);

    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }
    int port = atoi(argv[1]);
	if (!Utils::isValidPortArg(port)) {
		std::cerr << "Failure: Recommended port for a non encrypted IRC: 6667" << std::endl;
		return 1;
	}
	std::string password = argv[2];
	if (!Utils::isValidPasswordArg(password)) {
		std::cerr 	<< "Failure: invalid password" << std::endl;
		return 1;
	}
	Server server;
	try
	{
		server.serverInit(port, password);
	}
    catch (const std::exception &e)
    {
        server.closeFds();
        std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}