#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>

#include "../src/ir/nodes.hpp"
#include "../src/backend/ir_analyzer/ir_analyzer.hpp"
#include "../src/ir/input/parser.hpp"
#include "../src/ir/output/ir_emitter.hpp"
#include "../src/ir/input/lexer.hpp"
#include "../src/backend/debug/emitter_attachments.hpp"
#include "../src/backend/codegen/codegen.hpp"
#include "../src/exec/executor.hpp"

void io_stack_test() {
    std::ifstream file { "../examples/read_write_exact.ir" };

    if (!file.is_open()) {
        std::cerr << "Failed to open file read_write_exact.ir" << std::endl;
        exit(1);
    }

    std::string input { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    file.close();

    auto tokens = ir::lexer::lex(input);
    auto parsed = ir::parser::parse(tokens);

    std::stringstream ss;
    ir::output::emit(ss, parsed);

    auto new_ir = ss.str();
    auto new_tokens = ir::lexer::lex(new_ir);

    for (size_t i = 0; i < std::min(tokens.size(), new_tokens.size()); i++) {
        if (tokens[i].value == new_tokens[i].value)
            continue;

        std::cerr << "Token mismatch\n";
        std::cerr << "Expected: " << tokens[i].value << '\n';
        std::cerr << "Got: " << new_tokens[i].value << '\n';

        std::cout << ss.str() << std::endl;

        exit(1);
    }

    std::cout << "io_stack_test passed successfully!" << std::endl;
}

void hello_world_lex() {
    std::ifstream file { "../examples/cast_test.ir" };
    std::ofstream ofile { "../examples/cast_test.asm" };

    if (!file.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return;
    }

    std::string input { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    auto tokens = ir::lexer::lex(input);
    auto parsed = ir::parser::parse(tokens);
    backend::analyze_ir(parsed);

    ir::output::instruction_emitter_attachment = backend::output::attach_variable_drop;
    backend::codegen::generate(parsed,std::cout);

    file.close();
//    ofile.close();
//
//    exec::execute("../examples/cast_test.asm");

    asm("nop");
}

int main() {
    io_stack_test();
    hello_world_lex();
}
