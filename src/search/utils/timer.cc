#include "timer.h"

#include <ostream>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

Timer::Timer() {
    currentTime = getCurrentTime();
}

void Timer::reset() {
    currentTime = getCurrentTime();
}

double Timer::operator()() const {
    return getCurrentTime() - currentTime;
}

inline double Timer::getCurrentTime() const {
    timeval tv;
    gettimeofday(&tv, nullptr);
    return (double)tv.tv_sec + (double)tv.tv_usec / USEC_PER_SEC;
}

ostream& operator<<(ostream& os, const Timer& timer) {
    double value = timer();
    if (value < 0 && value > -1e-10)
        value = 0.0; // We sometimes get inaccuracies from god knows where.
    if (value < 1e-10)
        value = 0.0; // Don't care about such small values.
    os << value << "s";
    return os;
}
