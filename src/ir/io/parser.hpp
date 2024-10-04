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
}