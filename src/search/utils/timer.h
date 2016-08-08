#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <ostream>

// Measure amount of time elapsed since construction/reset.
class Timer {
public:
    void reset();
    // Returns the elapsed time since start.
    double operator()() const;

private:
    std::chrono::steady_clock::time_point startTime;
};

std::ostream& operator<<(std::ostream& os, const Timer& timer);

#endif
