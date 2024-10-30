/// Idea: Compare IR generated from input to itself re-ran through the lexer and parser

#include <fstream>
#include <string_view>
#include "../src/ir/input/parser.hpp"
#include "../src/ir/output/ir_emitter.hpp"

void test_consistency(std::string_view file_path) {
    std::ifstream file { file_path.begin() };

    if (!file.is_open()) {
        std::cerr << "Failed to open file " << file_path << '\n';
        exit(1);
    }

    std::string input { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    file.close();

    auto tokens = ir::lexer::lex(input);
    auto parsed = ir::parser::parse(tokens);

    std::stringstream ss;
    ir::output::emit(ss, parsed);

    auto ir2 = ss.str();
    auto lex2 = ir::lexer::lex(ir2);
    auto parsed2 = ir::parser::parse(tokens);

    ss.clear();

    ir::output::emit(ss, parsed);
    auto ir3 = ss.str();

    if (ir3 == ss.str()) {
        std::cout << "Parser Consistency Passed for " << file_path << '\n';
        return;
    }

    std::cerr << "Parser Inconsistency Found for " << file_path << '\n';
    std::cout << ir2 << '\n' << ir3 << '\n';
}

void run_parser_consistency_tests() {
    test_consistency("../examples/arith_select_test.ir");
    test_consistency("../examples/fibonacci.ir");
    test_consistency("../examples/hello_world.ir");
    test_consistency("../examples/phi_test.ir");
    test_consistency("../examples/read_write_exact.ir");
    test_consistency("../examples/select_test.ir");
}