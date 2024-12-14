#include "../src/backend/interface.hpp"

#include "test_utils.cpp"

void assert_dead_code_eliminated(std::string_view file_name, int branch_delta = -1) {
    auto ast = backend::gen_ast(file_name);

    size_t preopt_branches = 0;

    for (auto &func : ast.functions) {
        preopt_branches += func.blocks.size();
    }

    backend::opt::dead_code_elim(ast);

    size_t postopt_branches = 0;

    for (auto &func : ast.functions) {
        postopt_branches += func.blocks.size();
    }

    if (branch_delta == -1) {
        const auto debug_fail = [&]() {
            return std::string("No branches were eliminated in IR file: ").append(file_name);
        };

        debug::assert(postopt_branches < preopt_branches, debug_fail().c_str());
    } else {
        const auto expected_branches = preopt_branches - branch_delta;

        const auto debug_fail = [&]() {
            return std::string("No branches were eliminated in IR file: ").append(file_name)
                .append("Expected branches: ").append(std::to_string(expected_branches))
                .append("Actual branches: ").append(std::to_string(postopt_branches));
        };

        debug::assert(postopt_branches == expected_branches, debug_fail().c_str());
    }
}

void run_optimization_tests() {
    assert_dead_code_eliminated("../examples/optimizer/dead_code_elim.ir", 1);

    std::cout << "Optimization Tests Passed" << '\n';
}