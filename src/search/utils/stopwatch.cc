#include "stopwatch.h"

using namespace std::chrono;

void Stopwatch::reset() {
    startTime = steady_clock::now();
}

double Stopwatch::operator()() const {
    duration<double> time_span = steady_clock::now() - startTime;
    return time_span.count();
}

std::ostream& operator<<(std::ostream& os, Stopwatch const& stopwatch) {
    os << stopwatch() << "s";
    return os;
}
