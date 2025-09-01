#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

class Utils {
public:
    static bool isValidPortArg(int const& port);
    static bool isValidPasswordArg(const std::string& password);
};

#endif
