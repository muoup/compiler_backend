#include "parser.hpp"
#include "instruction_parsing.hpp"

namespace ir::parser {
    template <typename Arg>
    inline auto parse_argument(block::block_instruction &, parser::lex_iter_t &start, parser::lex_iter_t end) {
        throw std::runtime_error("Unknown argument type");
    }

    template <>
    inline auto parse_argument<uint8_t>(block::block_instruction &, parser::lex_iter_t &start, parser::lex_iter_t end) {
        return parse_uint8_t(start, end);
    }

    template <>
    inline auto parse_argument<ir::value>(block::block_instruction &, parser::lex_iter_t &start, parser::lex_iter_t end) {
        return parse_value(start, end);
    }

    template <>
    inline auto parse_argument<ir::block::icmp_type>(block::block_instruction &, parser::lex_iter_t &start, parser::lex_iter_t end) {
        return parse_icmp_type(start, end);
    }

    template <>
    inline auto parse_argument<std::string>(block::block_instruction &inst_wrapper, parser::lex_iter_t &start, parser::lex_iter_t end) {
        inst_wrapper.labels_referenced.emplace_back(start->value);
        return start++->value;
    }

    template <>
    inline auto parse_argument<ir::value_size>(block::block_instruction &, parser::lex_iter_t &start, parser::lex_iter_t end) {
        return parse_value_size(start, end);
    }

}