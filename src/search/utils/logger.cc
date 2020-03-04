#include "logger.h"

#ifdef NDEBUG
Verbosity Logger::runVerbosity = Verbosity::NORMAL;
#else
Verbosity Logger::runVerbosity = Verbosity::DEBUG;
#endif