#include "parser.hpp"
#include "../../debug/assert.hpp"
#include "instruction_parsing.hpp"

using namespace ir;

ir::root parser::parse(const std::vector<ir::lexer::token> &tokens) {
    return parse_root(tokens.begin(), tokens.end());
}

ir::root parser::parse_root(ir::parser::lex_iter_t start, ir::parser::lex_iter_t end) {
    ir::root root {};

    while (start < end) {
        while (start->type == lexer::token_type::break_line)
            ++start;

        debug::assert(start->type == lexer::token_type::identifier, "Expected identifier");

        if (start->value == "global_string") {
            root.global_strings.emplace_back(parse_global_string(++start, end));
        } else if (start->value == "extern") {
            root.extern_functions.emplace_back(parse_extern_function(++start, end));
        } else if (start->value == "define") {
            root.functions.emplace_back(parse_function(++start, end));
        } else {
            debug::assert(false, "Unknown global node type");
        }
    }

    return root;
}

ir::global::global_string parser::parse_global_string(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    debug::assert(start->value == "%", "Expected %");
    debug::assert((++start)->type == lexer::token_type::identifier, "Expected identifier");

    std::string name = start->value;

    debug::assert((++start)->value == "=", "Expected =");
    debug::assert((++start)->type == lexer::token_type::string, "Expected c\"");

    return ir::global::global_string {name, start++->value };
}

ir::global::extern_function parser::parse_extern_function(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    debug::assert((start++)->value == "fn", "Expected fn");

    // TODO: Function return type static analysis
    [[maybe_unused]]
    auto return_type = parse_value_size(start, end);

    debug::assert(start->type == lexer::token_type::identifier, "Expected identifier");

    auto name = start++->value;
    auto parameters = parse_parameters(start, end);

    debug::assert(start++->type == lexer::token_type::break_line, "Expected Break Line");

    return ir::global::extern_function { std::move(name), std::move(parameters) };
}

ir::global::function parser::parse_function(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    auto function_prototype = parse_extern_function(start, end);

    std::vector<ir::block::block> blocks;

    if (start->value != ".")
        blocks.emplace_back("entry");

    while (start->type == lexer::token_type::break_line)
        ++start;

    while (start->value != "end") {
        if (start->value == ".") {
            blocks.emplace_back((++start)->value);

            debug::assert((++start)->value == ":", "Expected Colon after Label");
            debug::assert((++start)->type == ir::lexer::token_type::break_line, "Expected Break Line");

            ++start;

            continue;
        }

        blocks.back().instructions.push_back(parse_instruction(start, end));

        while (start->type == lexer::token_type::break_line)
            ++start;
    }

    start++;

    return ir::global::function {
        std::move(function_prototype.name),
        std::move(function_prototype.parameters),
        std::move(blocks)
    };
}

std::vector<ir::variable> parser::parse_parameters(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    std::vector<ir::variable> parameters;

    debug::assert(start++->value == "(", "Expected (");

    if (start->value == ")") {
        start++;
        return parameters;
    }

    start--;

    do {
        start++;
        auto param = parse_value(start, end);

        if (std::holds_alternative<ir::int_literal>(param.val))
            throw std::runtime_error("Integers are not allowed as parameters");

        parameters.emplace_back(std::get<ir::variable>(param.val));
    } while (start->value == ",");

    debug::assert(start++->value == ")", "Expected )");

    return parameters;
}