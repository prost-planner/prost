#include "math.h"

#include <cmath>
#include <cstdlib>
#include <limits>

namespace prost::parser::utils {
bool doubleIsEqual(double const& d1, double const& d2) {
    return std::fabs(d1 - d2) < EPSILON;
}

bool doubleIsSmaller(double const& d1, double const& d2) {
    return d1 + EPSILON < d2;
}

bool doubleIsGreater(double const& d1, double const& d2) {
    return d1 > d2 + EPSILON;
}

bool doubleIsSmallerOrEqual(double const& d1, double const& d2) {
    return !doubleIsGreater(d1, d2);
}

bool doubleIsGreaterOrEqual(double const& d1, double const& d2) {
    return !doubleIsSmaller(d1, d2);
}

bool doubleIsMinusInfinity(double const& d1) {
    return doubleIsEqual(d1, -std::numeric_limits<double>::max());
}

bool multiplyWithOverflowCheck(long& x, unsigned long const& y) {
    long base = x;
    x = 0;
    for (unsigned long i = 0; i < y; ++i) {
        x += base;
        if (x < base) {
            return false;
        }
    }
    return true;
}

double generateRandomNumber() {
    return (double)(rand() % 1000001) / 1000001.0;
}
} // namespace prost::parser::utils
