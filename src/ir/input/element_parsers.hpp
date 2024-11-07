#include "parser.hpp"
#include "instruction_parsing.hpp"

namespace ir::parser {
    template <typename Arg>
    inline auto parse_argument(parser::lex_iter_t &start, parser::lex_iter_t end) {
        throw std::runtime_error("Unknown argument type");
    }

    template <>
    inline auto parse_argument<uint8_t>(parser::lex_iter_t &start, parser::lex_iter_t end) {
        return parse_uint8_t(start, end);
    }

    template <>
    inline auto parse_argument<ir::value>(parser::lex_iter_t &start, parser::lex_iter_t end) {
        return parse_value(start, end);
    }

    template <>
    inline auto parse_argument<ir::block::icmp_type>(parser::lex_iter_t &start, parser::lex_iter_t end) {
        return parse_icmp_type(start, end);
    }

    template <>
    inline auto parse_argument<std::string>(parser::lex_iter_t &start, parser::lex_iter_t end) {
        return start++->value;
    }

    template <>
    inline auto parse_argument<ir::value_size>(parser::lex_iter_t &start, parser::lex_iter_t end) {
        return parse_value_size(start, end);
    }

}