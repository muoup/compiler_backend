#include <fstream>
#include <iostream>
#include <sstream>

#include "../src/backend/interface.hpp"
#include "../src/exec/executor.hpp"

void assert_file_exitcode(const char* file_path, int exit_code) {
    auto ast = backend::gen_ast(file_path);

    std::stringstream ss;
    std::ofstream output { "../examples/output.asm" };

    backend::compile(ast, ss);
    output << ss.str();
    output.close();

    auto exit = exec::run_once("../examples/output.asm");

    if (exit != exit_code) {
        std::cerr << "Error in file " << file_path << '\n';
        std::cerr << "Expected exit code " << exit_code << " but got " << exit << '\n';
        std::cout << "Received output:\n";
        std::cout << ss.str();
        std::exit(1);
    }
}

void run_exec_tests() {
    assert_file_exitcode("../examples/arith_select_test.ir", 5);
    assert_file_exitcode("../examples/select_test.ir", 1);
    assert_file_exitcode("../examples/fibonacci.ir", 55);
    assert_file_exitcode("../examples/pointer_test.ir", 2);

    std::cout << "All execution tests passed\n";
}