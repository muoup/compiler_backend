#include "interface.hpp"

#include <iostream>
#include <fstream>

#include "ir_analyzer/ir_analyzer.hpp"
#include "codegen/codegen.hpp"
#include "../ir/input/lexer.hpp"
#include "../ir/input/parser.hpp"

std::vector<ir::lexer::token> backend::lex(std::string_view file_name) {
    std::ifstream file { file_name.begin() };

    if (!file.is_open()) {
        std::cerr << "Failed to open file " << file_name << '\n';
        exit(1);
    }

    std::string input { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    return ir::lexer::lex(input);
}

ir::root backend::gen_ast(std::string_view file_name) {
    auto lex = backend::lex(file_name);
    return ir::parser::parse(lex);
}

void backend::compile(ir::root &root, std::ostream &ostream) {
    analyze_ir(root);
    backend::context::generate(root, ostream);
}

void backend::compile(std::string_view file_name, std::ostream &ostream) {
    auto ast = backend::gen_ast(file_name);
    backend::compile(ast, ostream);
}

void backend::analyze_ir(ir::root &root) {
    backend::md::analyze(root);
}