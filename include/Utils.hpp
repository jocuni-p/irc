#ifndef UTILS_HPP
#define UTILS_HPP


#include <vector>      // Para std::vector<>
#include <string>      // Para std::string
#include <sstream>     // Para std::stringstream
#include <iostream>    // Para std::getline (opcional, pero recomendado)


#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
//#define YELLOW "\033[33m"
#define YELLOW_SOFT "\033[38;5;228m"
#define YELLOW_PALE "\033[38;5;229m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
// #define RED "\e[1;31m"
// #define WHI "\e[0;37m"
// #define GRE "\e[1;32m"
// #define YEL "\e[1;33m"


class Utils {
	public:
		static bool isValidPortArg(int const& port);
		static bool isValidPasswordArg(const std::string& password);
		static std::vector<std::string> split(const std::string &str, char delim);
		static std::string stripCRLF(const std::string &msg);
		static std::string joinFrom(const std::vector<std::string> &v, size_t start, const std::string &sep = " ");
};

#endif
