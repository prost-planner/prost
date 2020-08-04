#ifndef TIMER_H
#define TIMER_H

#define USEC_PER_SEC (1000000)

#include <iosfwd>

namespace prost {
namespace parser {
namespace utils {
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
} // namespace utils
} // namespace parser
} // namespace prost

#endif
