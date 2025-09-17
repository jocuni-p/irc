#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

enum LoginStatus {
    NOT_AUTHENTICATED,
	CAP_NEGOTIATED,
    PASS_OK,
    NICK_OK,
	NICK_AGAIN,
    USER_OK,
    AUTHENTICATED
};


class Client
{
private:
    int             _fd;
    std::string 	_ip;       
    std::string 	_nickname;
    std::string 	_username;
    std::string 	_realname;
    std::string     _buffer;
	LoginStatus 	_status;
	bool 			_toBeRemoved;

public:
    Client();
    ~Client();

    // --- Getters ---
    int             getFd() const;
    std::string     getIp() const;
    std::string     getNickname() const;
    std::string     getUsername() const;
    std::string     getRealname() const;
	LoginStatus 	getStatus() const;
	std::string 	&getBuffer();

	// --- Setters ---
    void            setFd(int newFd);
    void            setIp(const std::string& newIp);
    void            setNickname(const std::string& nick);
    void            setUsername(const std::string& user);
    void            setRealname(const std::string& real);
	void 			setStatus(LoginStatus s);

	void 			markForRemoval();
	bool 			isMarkedForRemoval() const;

	// --- Buffer handling ---
    void            appendToBuffer(const std::string& data);
    void            clearBuffer();
};

#endif
