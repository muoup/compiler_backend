#include "lexer_test.cpp"
#include "parser_consistency_tests.cpp"
#include "execution_tests.cpp"
#include "optimization_tests.cpp"

#include "../src/backend/ir_optimizer/dead_code_elim.hpp"

int main() {
    run_lexer_tests();
    run_exec_tests();
    run_parser_consistency_tests();
    run_optimization_tests();
}
