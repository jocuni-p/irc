#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <csignal> // para sig_atomic_t

// Para que se pueda ver esta variable desde Server.cpp
extern volatile sig_atomic_t g_running;

#endif
