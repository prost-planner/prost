#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#define EPSILON std::numeric_limits<double>::epsilon()

#include <cstdlib>
#include <cmath>
#include <vector>
#include <cassert>
#include <limits>

class MathUtils {
public:
    static bool doubleIsEqual(double const& d1, double const& d2) {
        return std::fabs(d1-d2) < EPSILON;
    }

    static bool doubleIsSmaller(double const& d1, double const& d2) {
        return d1+EPSILON < d2;
    }

    static bool doubleIsGreater(double const& d1, double const& d2) {
        return d1 > d2+EPSILON;
    }

    static bool doubleIsSmallerOrEqual(double const& d1, double const& d2) {
        return !doubleIsGreater(d1,d2);
    }

    static bool doubleIsGreaterOrEqual(double const& d1, double const& d2) {
        return !doubleIsSmaller(d1,d2);
    }

    static bool doubleIsMinusInfinity(double const& d1) {
        return doubleIsEqual(d1,-std::numeric_limits<double>::max());
    }

    static bool multiplyWithOverflowCheck(long& x, unsigned int const& y) {
        long base = x;
        x = 0;
        for(unsigned int i = 0; i < y; ++i) {
            x += base;
            if(x < base) {
                return false;
            }
        }
        return true;
    }

    static double generateRandomNumber() {
        return ((double)(rand() % 1000001) / 1000000.0);
    }

private:
    MathUtils() {}
};

#endif
