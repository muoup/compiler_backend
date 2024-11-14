#include <string>
#include <iostream>
#include "executor.hpp"

std::string get_nasm_command(const char *file) {
    return "nasm -g -f elf64 -o " + std::string(file) + ".o " + file;
}

std::string get_gcc_command(const char *file) {
    return "gcc -no-pie -z noexecstack -o " + std::string(file) + ".out " + file + ".o";
}

std::string get_run_command(const char *file) {
    return "./" + std::string(file) + ".out";
}

std::string get_clean_command(const char *file) {
    return "rm " + std::string(file) + ".o" + " " + std::string(file) + ".asm";
}

void sys(const char *cmd) {
    std::system(cmd);
}

int exec::execute(const char *file) {
    sys(get_nasm_command(file).c_str());
    sys(get_gcc_command(file).c_str());
    sys(get_clean_command(file).c_str());

    return WEXITSTATUS(system(get_run_command(file).c_str()));
}