#pragma once

#include "parser.hpp"
#include "lexer.hpp"

namespace ir::parser {
    ir::block::block_instruction parse_instruction(lex_iter_t &start, lex_iter_t end);
    ir::block::block_instruction parse_unassigned_instruction(parser::lex_iter_t &start, parser::lex_iter_t end);

    uint8_t parse_uint8_t(lex_iter_t &start, lex_iter_t end);

    std::optional<ir::value_size> maybe_value_size(lex_iter_t &start, lex_iter_t end);
    ir::value_size parse_value_size(lex_iter_t &start, lex_iter_t end);

    ir::value parse_value(lex_iter_t &start, lex_iter_t end);
    ir::variable parse_variable(lex_iter_t &start, lex_iter_t end, ir::value_size size = ir::value_size::none);
    ir::block::icmp_type parse_icmp_type(lex_iter_t &start, lex_iter_t end);

    std::vector<value> parse_operands(lex_iter_t &start, lex_iter_t end);
}