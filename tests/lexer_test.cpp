#include <iostream>
#include "../src/ir/input/lexer.hpp"
#include "../src/debug/assert.hpp"

void compare_lex(const std::vector<ir::lexer::token> &output,
                 const std::vector<ir::lexer::token> &expected_output) {
    debug::assert(output.size() == expected_output.size(), "expect_lex: wrong # of outputted symbols");

    for (size_t i = 0; i < output.size(); i++) {
        const auto &tok = output[i];
        const auto &expected_tok = expected_output[i];

        const auto error_msg = [&]() {
            return std::string("expect_lex: expected ")
                    .append(expected_tok.value)
                    .append(", got ")
                    .append(tok.value);
        };

        debug::assert(tok.type == expected_tok.type && tok.value == expected_tok.value,
                      error_msg().c_str());
    }
}

void expect_lex(std::string_view input, const std::vector<ir::lexer::token>& expected_output) {
    const auto output = ir::lexer::lex(input);

    compare_lex(output, expected_output);
}

void lexer_test0() {
    using enum ir::lexer::token_type;

    expect_lex("%1 = add %2, %3", std::vector<ir::lexer::token> {
        { symbol, "%" },
        { number, "1" },
        { symbol, "=" },
        { identifier, "add" },
        { symbol, "%" },
        { number, "2" },
        { symbol, "," },
        { symbol, "%" },
        { number, "3" }
    });

    std::cout << "lexer_test0 passed!" << '\n';
}

// TODO: More Lexer Tests

void run_lexer_tests() {
    lexer_test0();
}