#include <fstream>
#include <iostream>
#include "../src/backend/interface.hpp"
#include "../src/exec/executor.hpp"

void assert_file_exitcode(const char* file_path, int exit_code) {
    std::ifstream file { file_path };
    std::ofstream output { "../examples/output.asm" };

    if (!file.is_open()) {
        std::cerr << "Failed to open file " << file_path << '\n';
        exit(1);
    }

    std::string input { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    file.close();

    auto lex = ir::lexer::lex(input);
    auto ast = ir::parser::parse(lex);

    backend::compile(ast, output);
    output.close();

    debug::assert(exec::run_once("../examples/output.asm") == exit_code, "Exit code does not match");
}

void run_exec_tests() {
    assert_file_exitcode("../examples/fibonacci.ir", 55);

    std::cout << "All execution tests passed\n";
}