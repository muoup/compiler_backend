#include <fstream>
#include <iostream>
#include <sstream>

#include "src/ir/input/lexer.hpp"
#include "src/ir/input/parser.hpp"
#include "src/backend/interface.hpp"
#include "src/exec/executor.hpp"

int main() {
    const char *file_path = "../examples/cast_test.ir";

    std::ifstream file { file_path };
    std::ofstream output { "../output.asm" };

    std::string input { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    file.close();

    auto lex = ir::lexer::lex(input);
    auto ast = ir::parser::parse(lex);

    std::stringstream ss;

    backend::compile(ast, ss);
    output << ss.str();
    std::cout << ss.str();

    output.close();
    std::cout << "\nCompilation complete.\n";
    std::cout << "Program Output:\n";

//    return exec::run_once("../output.asm");
}