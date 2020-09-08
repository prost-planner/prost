#include "system.h"

#include <iostream>

namespace prost::parser::utils {
void abort(std::string msg) {
    std::cerr << "\033[1;31mERROR! \033[0m" << msg << std::endl;
    exit(0);
}
} // namespace prost::parser::utils
