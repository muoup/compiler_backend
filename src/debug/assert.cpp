#include <iostream>
#include "assert.hpp"

void debug::assert(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << std::endl;
        throw std::runtime_error("Assertion failed");
    }
}
