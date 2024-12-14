#pragma once

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

void compile_and_cout(std::string_view file_name) {
    auto ast = backend::gen_ast(file_name);

    backend::compile(ast, std::cout);
}

void optimize_and_cout(std::string_view file_name, void(*optimizer)(ir::root&)) {
    auto ast = backend::gen_ast(file_name);
    optimizer(ast);

    ir::output::emit(ast, std::cout);
}