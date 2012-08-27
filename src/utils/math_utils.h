#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#define EPSILON 0.0001

#include <cmath>
#include <vector>
#include <cassert>
#include <limits>

class MathUtils {
public:
    static bool doubleIsEqual(const double& d1, const double& d2) {
        return std::fabs(d1-d2) < EPSILON;
    }

    static bool doubleIsSmaller(const double& d1, const double& d2) {
        return d1+EPSILON < d2;
    }

    static bool doubleIsGreater(const double& d1, const double& d2) {
        return d1 > d2+EPSILON;
    }

    static bool doubleIsSmallerOrEqual(const double& d1, const double& d2) {
        return !doubleIsGreater(d1,d2);
    }

    static bool doubleIsGreaterOrEqual(const double& d1, const double& d2) {
        return !doubleIsSmaller(d1,d2);
    }

    static bool doubleIsMinusInfinity(const double& d1) {
        return doubleIsEqual(d1,-std::numeric_limits<double>::max());
    }

    static void initTwoToThePowerOfMap();
    static void initThreeToThePowerOfMap();

    static unsigned long const& twoToThePowerOf(unsigned int& n) {
        assert(n < twoToThePowerOfMap.size());
        return twoToThePowerOfMap[n];
    }

    static unsigned long const& twoToThePowerOf(int& n) {
        assert(n < twoToThePowerOfMap.size());
        return twoToThePowerOfMap[n];
    }

    static unsigned long const& threeToThePowerOf(unsigned int& n) {
        assert(n < threeToThePowerOfMap.size());
        return threeToThePowerOfMap[n];
    }

    static unsigned long const& threeToThePowerOf(int& n) {
        assert(n < threeToThePowerOfMap.size());
        return threeToThePowerOfMap[n];
    }

    static std::vector<unsigned long> twoToThePowerOfMap;
    static std::vector<unsigned long> threeToThePowerOfMap;

private:
    MathUtils() {}
};

#endif
