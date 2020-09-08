#ifndef PARSER_UTILS_TIMER_H
#define PARSER_UTILS_TIMER_H

#define USEC_PER_SEC (1000000)

#include <iosfwd>

namespace prost::parser::utils {
class Timer {
public:
    Timer();
    ~Timer() {}
    void reset();
    double operator()() const;
    double getCurrentTime() const;

private:
    double currentTime;
};

std::ostream& operator<<(std::ostream& os, const Timer& timer);
} // namespace prost::parser::utils

#endif // PARSER_UTILS_TIMER_H
