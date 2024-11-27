#pragma once

namespace exec {
    int execute(std::string_view file);
    int run_once(std::string_view file);
    void print_asm(std::string_view file);
}
