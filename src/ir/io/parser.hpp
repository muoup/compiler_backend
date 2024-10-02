#pragma once

#include <vector>
#include "lexer.hpp"
#include "../nodes.hpp"

namespace ir::parser {
    using lex_iter_t = std::vector<lexer::token>::const_iterator;

    ir::root parse(const std::vector<ir::lexer::token>& tokens);

    static ir::root parse_root(lex_iter_t start, lex_iter_t end);

    static ir::global::global_string parse_global_string(lex_iter_t &start, lex_iter_t end);
    static ir::global::extern_function parse_extern_function(lex_iter_t &start, lex_iter_t end);
    static ir::global::function parse_function(lex_iter_t &start, lex_iter_t end);

    static ir::block::block_instruction parse_instruction(lex_iter_t &start, lex_iter_t end);

    // Instructions
    static ir::block::call parse_call(lex_iter_t &start, lex_iter_t end);
    static ir::block::ret parse_ret(lex_iter_t &start, lex_iter_t end);

    // Value
    static ir::value parse_value(lex_iter_t &start, lex_iter_t end);
    static ir::variable parse_variable(lex_iter_t &start, lex_iter_t end);
}