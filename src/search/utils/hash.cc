#include "hash.h"

using namespace std;

namespace utils {
namespace {
inline unsigned int hashVectors(vector<double> const& v1,
                                vector<double> const& v2, unsigned int& mult) {
    unsigned int hashValue = 0x345678;
    int i = static_cast<int>(v1.size()) - 1;
    for (; i >= 0; --i) {
        hashValue = (hashValue ^ (static_cast<int>(v1[i]))) * mult;
        mult += 82520 + i + i;
    }
    i = static_cast<int>(v2.size()) - 1;
    for (; i >= 0; --i) {
        hashValue = (hashValue ^ (static_cast<int>(v2[i]))) * mult;
        mult += 82520 + i + i;
    }
    return hashValue;
}
}

unsigned int hash(vector<double> const& v1, vector<double> const& v2) {
    unsigned int mult = 1000003;
    return hashVectors(v1, v2, mult) + 97531;
}

unsigned int hash(vector<double> const& v1, vector<double> const& v2, int n) {
    unsigned int mult = 1000003;
    unsigned int hashValue = hashVectors(v1, v2, mult);
    hashValue = (hashValue ^ n) * mult;
    return hashValue + 97531;
}
}