#ifndef UTILS_HASH_H
#define UTILS_HASH_H

/*
  This implementation of hash functions for hashing of vectors has been written
  by ourselves, but the underlying logic has been taken from an (presumably
  outdated) implementation of hashing tuples in python.
*/

#include <vector>

namespace utils {
extern unsigned int hash(std::vector<double> const& v1,
                         std::vector<double> const& v2);
extern unsigned int hash(std::vector<double> const& v1,
                         std::vector<double> const& v2, int n);
}
#endif // UTILS_HASH_H
