#ifndef LOGGER_H
#define LOGGER_H

/*
 This is a simple class for logging that supports four different log levels.
 - SUPPRESS is a log level that must not be used as runVerbosity. It is used
   for output that should never be printed.
 - SILENT is the log level that is used in experiments, where the amount of
   output should be kept to a minimum. This log level is not intended for human
   readable output, so don't add unnecessary blank lines, seperators etc. Only
   log information that is processed by an experiment parser.
 - NORMAL is the log level that is targeted at local runs in release mode,
   where the planner is typically run only for a few steps. Print information
   that is relevant for most issues on this log level, but make sure to keep
   the output clear.
 - VERBOSE is also targeted at local runs in release mode where the planner is
   run only for a few steps, but it prints a lot more information than the NORMAL
   log level.
 - DEBUG is the log level that is only for debugging an ongoing implementation.
   Creating a log level for this allows to have debug output locally and test the
   implementation in an experiment without having comment / uncomment debug output
   all the time. However, before an issue can  be merged, all DEBUG output must be
   removed.
 */

#include <string>

enum class Verbosity {
    SUPPRESS,
    SILENT,
    NORMAL,
    VERBOSE,
    DEBUG
};

struct Logger {
    static Verbosity runVerbosity;

    // prints message plus an endline to stdout if the verbosity of this
    // run is at least as high as the given verbosity
    static void logLine(std::string const &message = "",
                        Verbosity verbosity = Verbosity::VERBOSE);

    // prints message to stdout if the verobosity of this run is at least
    // as high as the given verbosity, and otherwise logs altMessage if the
    // verbosity is at least as the given altVerbosity
    static void logLineIf(
        std::string const &message, Verbosity verbosity,
        std::string const &altMessage, Verbosity altVerbosity);

    // prints message to stdout if the verbosity of this
    // run is at least as high as the given verbosity
    static void log(std::string const &message,
                    Verbosity verbosity = Verbosity::VERBOSE);

    // prints a separator to stdout if the verbosity of this
    // run is at least as high as the given verbosity
    static void logSeparator(Verbosity verbosity = Verbosity::VERBOSE);

    // prints a small separator to stdout if the verbosity of this
    // run is at least as high as the given verbosity
    static void logSmallSeparator(Verbosity verbosity = Verbosity::VERBOSE);

    // prints an error message to stderr
    static void logError(std::string const &message);

    // prints a warning to stderr
    static void logWarning(std::string const &message);
};


#endif
