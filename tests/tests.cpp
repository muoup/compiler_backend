#include <iostream>
#include <memory>
#include <fstream>
#include "../src/ir/nodes.hpp"
#include "../src/ir/io/lexer.hpp"
#include "../src/ir/io/parser.hpp"

void hello_world_lex() {
    std::ifstream file { "../examples/hello_world.ir" };

    if (!file.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return;
    }

    std::string input { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    auto tokens = ir::lexer::lex(input);
    auto parsed = ir::parser::parse(tokens);

//    codegen::generate(parsed, std::cout);
}

int main() {
    hello_world_lex();
}
