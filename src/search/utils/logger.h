#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

enum class Verbosity {
    SILENT,
    NORMAL,
    VERBOSE,
    DEBUG
};

struct Logger {
    static Verbosity runVerbosity;

    // prints message to stdout if the verbosity of this run is
    // at least as high as the verbosity of this message
    static void logLine(std::string const &message = "",
                        Verbosity verbosity = Verbosity::VERBOSE) {
        if (runVerbosity >= verbosity) {
            std::cout << message << std::endl;
        }
    }

    static void log(std::string const &message,
                    Verbosity verbosity = Verbosity::VERBOSE) {
        if (runVerbosity >= verbosity) {
            std::cout << message;
        }
    }

    static void logSeparator(Verbosity verbosity = Verbosity::VERBOSE) {
        logLine("------------------------------------------------", verbosity);
    }

    static void logSmallSeparator(Verbosity verbosity = Verbosity::VERBOSE) {
        logLine("----------------------", verbosity);
    }
};


#endif
