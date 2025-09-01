#include "../include/Server.hpp"

int main(int argc, char **argv)
{
    signal(SIGINT, Server::signalHandler);
    signal(SIGQUIT, Server::signalHandler);

    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return (1);
    }

    int port = atoi(argv[1]);
    std::string password = argv[2];

    Server server;
    std::cout << "---- IRC SERVER ----" << std::endl;

    try
    {
//        signal(SIGINT, Server::signalHandler);
//        signal(SIGQUIT, Server::signalHandler);
        server.serverInit(port, password);
    }
    catch (const std::exception &e)
    {
        server.closeFds();
        std::cerr << e.what() << std::endl;
    }
    std::cout << "The Server Closed!" << std::endl;
}