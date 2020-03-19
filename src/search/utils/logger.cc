#include "logger.h"

#include <iostream>

#ifdef NDEBUG
Verbosity Logger::runVerbosity = Verbosity::NORMAL;
#else
Verbosity Logger::runVerbosity = Verbosity::DEBUG;
#endif

void Logger::logLine(std::string const &message, Verbosity verbosity) {
    if (runVerbosity >= verbosity) {
        std::cout << message << std::endl;
    }
}

void Logger::logLineIf(
    std::string const &message, Verbosity verbosity,
    std::string const &altMessage, Verbosity altVerbosity) {
    if (runVerbosity >= verbosity) {
        Logger::logLine(message, verbosity);
    } else {
        Logger::logLine(altMessage, altVerbosity);
    }
}

void Logger::log(std::string const &message, Verbosity verbosity) {
    if (runVerbosity >= verbosity) {
        std::cout << message;
    }
}

void Logger::logSeparator(Verbosity verbosity) {
    Logger::logLine("--------------------------------------------", verbosity);
}

void Logger::logSmallSeparator(Verbosity verbosity) {
    Logger::logLine("----------------------", verbosity);
}

// prints an error message to stderr
void Logger::logError(std::string const &message) {
    std::cerr << "ERROR: " << message << std::endl;
}

// prints a warning to stderr
void Logger::logWarning(std::string const &message) {
    std::cerr << "WARNING: " << message << std::endl;
}

