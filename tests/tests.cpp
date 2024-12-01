#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>

#include "lexer_test.cpp"
#include "execution_tests.cpp"

#include "../src/backend/interface.hpp"
#include "../src/ir/input/parser.hpp"

void compile_and_cout(std::string_view file_name) {
    std::ifstream file { file_name.begin() };

    if (!file.is_open()) {
        std::cerr << "Failed to open file " << file_name << '\n';
        exit(1);
    }

    std::string input { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    auto lex = ir::lexer::lex(input);
    auto ast = ir::parser::parse(lex);

    backend::compile(ast, std::cout);
}

int main() {
//    compile_and_cout("../examples/fibonacci.ir");

        run_exec_tests();
}
