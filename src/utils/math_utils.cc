#include "math_utils.h"

std::vector<unsigned long> MathUtils::twoToThePowerOfMap;
std::vector<unsigned long> MathUtils::threeToThePowerOfMap;

void MathUtils::initTwoToThePowerOfMap() {
    MathUtils::twoToThePowerOfMap.push_back(1);
    unsigned long last = 1;

    while(true) {
        unsigned long nxt = last * 2;
        if(nxt > last) {
            MathUtils::twoToThePowerOfMap.push_back(nxt);
            last = nxt;
        } else {
            break;
        }
    }
}

void MathUtils::initThreeToThePowerOfMap() {
    MathUtils::threeToThePowerOfMap.push_back(1);
    unsigned long last = 1;
    
    while(true) {
        unsigned long nxt = last * 3;
        if(nxt > last) {
            MathUtils::threeToThePowerOfMap.push_back(nxt);
            last = nxt;
        } else {
            break;
        }
    }
}


