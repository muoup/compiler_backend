#pragma once
#include "parser.hpp"

namespace ir::parser {
    ir::block::block_instruction parse_instruction(lex_iter_t &start, lex_iter_t end);

    static ir::block::block_instruction parse_unassigned_instruction(parser::lex_iter_t &start, parser::lex_iter_t end);

    static uint8_t parse_uint8_t(lex_iter_t &start, lex_iter_t end);
    static ir::value parse_value(lex_iter_t &start, lex_iter_t end);
    static ir::variable parse_variable(lex_iter_t &start, lex_iter_t end);
    static ir::block::icmp_type parse_icmp_type(lex_iter_t &start, lex_iter_t end);

    static std::vector<value> parse_operands(lex_iter_t &start, lex_iter_t end);

    // Non-standard instruction parsing
    static ir::block::block_instruction parse_branch(lex_iter_t &start, lex_iter_t end);

    template <typename Arg>
    inline auto parse_argument(lex_iter_t &start, lex_iter_t end) {
        static_assert(false, "Unknown argument type");
    }

    template <>
    inline auto parse_argument<uint8_t>(lex_iter_t &start, lex_iter_t end) {
        return parse_uint8_t(start, end);
    }

    template <>
    inline auto parse_argument<ir::value>(lex_iter_t &start, lex_iter_t end) {
        return parse_value(start, end);
    }

    template <>
    inline auto parse_argument<ir::block::icmp_type>(lex_iter_t &start, lex_iter_t end) {
        return parse_icmp_type(start, end);
    }

    template <>
    inline auto parse_argument<std::string>(lex_iter_t &start, lex_iter_t end) {
        return start++->value;
    }
}