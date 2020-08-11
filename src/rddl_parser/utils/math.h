#ifndef PARSER_UTILS_MATH_H
#define PARSER_UTILS_MATH_H

#define EPSILON 0.000000001

namespace prost::parser::utils {
/*
  Methods for double comparison
*/
bool doubleIsEqual(double const& d1, double const& d2);
bool doubleIsSmaller(double const& d1, double const& d2);
bool doubleIsGreater(double const& d1, double const& d2);
bool doubleIsSmallerOrEqual(double const& d1, double const& d2);
bool doubleIsGreaterOrEqual(double const& d1, double const& d2);
bool doubleIsMinusInfinity(double const& d1);

/*
  Safe multiplication that returns 'false' in the case of an overflow
*/
bool multiplyWithOverflowCheck(long& x, unsigned long const& y);

/*
  Returns a random number in [0; 1)
  TODO: We should ensure that this always generates a number that is smaller
   than 1.0 - EPSILON
*/
double generateRandomNumber();
} // namespace prost::parser::utils

#endif // PARSER_UTILS_MATH_H
