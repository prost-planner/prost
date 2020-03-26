#include "math_utils.h"
std::unique_ptr<Random<>> MathUtils::rnd{new RandomMT()};

void MathUtils::resetRNG() {
    rnd.reset(new RandomMT());
}
