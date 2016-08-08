#include "timer.h"

using namespace std::chrono;

void Timer::reset() {
    startTime = steady_clock::now();
}

double Timer::operator()() const {
    duration<double> time_span = steady_clock::now() - startTime;
    return time_span.count();
}

std::ostream& operator<<(std::ostream& os, const Timer& timer) {
    os << timer() << "s";
    return os;
}
