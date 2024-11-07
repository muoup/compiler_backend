#pragma once

#include <vector>
#include "../nodes.hpp"

namespace ir::lexer {
    struct token;
}

namespace ir::parser {
    using lex_iter_t = std::vector<lexer::token>::const_iterator;

    ir::root parse(const std::vector<ir::lexer::token>& tokens);

    ir::root parse_root(lex_iter_t start, lex_iter_t end);

    ir::global::global_string parse_global_string(lex_iter_t &start, lex_iter_t end);
    ir::global::extern_function parse_extern_function(lex_iter_t &start, lex_iter_t end);
    ir::global::function parse_function(lex_iter_t &start, lex_iter_t end);

    std::vector<ir::variable> parse_parameters(lex_iter_t &start, lex_iter_t end);
}