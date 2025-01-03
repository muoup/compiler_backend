#include "lexer_test.cpp"
#include "parser_consistency_tests.cpp"
#include "execution_tests.cpp"
#include "optimization_tests.cpp"

void run_tests() {
    std::cout << "Running tests...\n";

    run_lexer_tests();
    run_exec_tests();

    std::cout << "Tests complete.\n";
}