#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <unistd.h>
#include <queue>

enum LoginStatus {
    NOT_AUTHENTICATED,
    PASS_OK,
    NICK_OK,
    USER_OK,
    REGISTERED
};


class Client {
private:
    int         _fd;
    std::string _ip;
    std::string _nickname;
    std::string _username; //<username> <hostname> <servername> <realname>
	LoginStatus _status; // 0=no autenticado, 1=pass ok, 2=nick ok, 3=user ok, 4=autenticado
    std::string _buffer;  // buffer acumulativo de datos recibidos
	
public:
	Client();
	Client(int fd);

	~Client();

	// --- Getters / Setters básicos ---
	int getFd() const;
	void setFd(int fd);

	const std::string &getIp() const;
	void setIp(const std::string &ip);

	const std::string &getNick() const;
	void setNick(const std::string &nick);

	const std::string &getUser() const;
	void setUser(const std::string &user);

	LoginStatus getStatus() const;
	void setStatus(LoginStatus s);

	// --- Buffer de entrada ---
	void appendToBuffer(const std::string &data);

	std::string &getBuffer();
	void clearBuffer();
};

#endif

