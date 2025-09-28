#ifndef UTILS_HPP
#define UTILS_HPP


#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include "Server.hpp"



#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW_SOFT "\033[38;5;228m"
#define YELLOW_PALE "\033[38;5;229m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"


class Utils {
	public:
		static bool isValidPortArg(int const& port);
		static bool isValidPasswordArg(const std::string& password);
		static std::vector<std::string> split(const std::string& str, char delim);
		static std::string stripCRLF(const std::string &msg);
		static std::string joinFrom(const std::vector<std::string> &v, size_t start, const std::string &sep = " ");
};

#endif
