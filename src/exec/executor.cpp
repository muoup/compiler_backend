#include <string>
#include <iostream>
#include "executor.hpp"
#include "../debug/assert.hpp"

std::string get_nasm_command(std::string_view file) {
    return std::string("nasm -g -f elf64 -o ")
        .append(file).append(".o ").append(file).append(".asm");
}

std::string get_gcc_command(std::string_view file) {
    return std::string("gcc -no-pie -z noexecstack -o ")
        .append(file).append(".out ").append(file).append(".o");
}

std::string get_run_command(std::string_view file) {
    return std::string("./")
        .append(file).append(".out");
}

std::string get_clean_command(std::string_view file) {
    return std::string("rm ")
        .append(file).append(".o ").append(file).append(".asm");
}

std::string get_clean_exec_command(std::string_view file) {
    return std::string("rm ")
        .append(file).append(".out");
}

int sys(std::string_view cmd) {
    return std::system(cmd.data());
}

std::string_view get_extensionless_filename(std::string_view file) {
    return file.substr(0, file.rfind("."));
}

int run_routine(std::string_view extensionless_path) {
    if (WEXITSTATUS(sys(get_nasm_command(extensionless_path))))
        return -1;
    sys(get_gcc_command(extensionless_path));
    sys(get_clean_command(extensionless_path));

    return WEXITSTATUS(sys(get_run_command(extensionless_path)));
}

int exec::execute(std::string_view file) {
    file = get_extensionless_filename(file);
    return run_routine(file);
}

int exec::run_once(std::string_view file) {
    file = get_extensionless_filename(file);
    auto exit_code = run_routine(file);
    sys(get_clean_exec_command(file));

    return exit_code;
}