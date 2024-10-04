#include "parser.hpp"
#include "../../debug/assert.hpp"
#include "instruction_parsing.hpp"

using namespace ir;

ir::root parser::parse(const std::vector<ir::lexer::token> &tokens) {
    return parse_root(tokens.begin(), tokens.end());
}

ir::root parser::parse_root(ir::parser::lex_iter_t start, ir::parser::lex_iter_t end) {
    ir::root root {};

    while (start != end) {
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

ir::global::extern_function parser::parse_extern_function(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t) {
    debug::assert((start++)->value == "fn", "Expected fn");
    debug::assert(start->type == lexer::token_type::identifier, "Expected identifier");

    auto return_type = start->value;

    debug::assert((++start)->type == lexer::token_type::identifier, "Expected identifier");

    auto name = start->value;

    debug::assert((++start)->value == "(", "Expected (");

    std::vector<ir::global::parameter> parameters {};

    if ((++start)->value != ")") {
        do {
            using enum ir::global::parameter_type;

            ir::global::parameter_type type;

            if (start->value == "i8")       type = i8;
            else if (start->value == "i16") type = i16;
            else if (start->value == "i32") type = i32;
            else if (start->value == "i64") type = i64;
            else if (start->value == "ptr") type = ptr;
            else debug::assert(false, "Unknown parameter type");

            std::string parameter_name;

            if ((++start)->value == "%") {
                debug::assert((++start)->type == lexer::token_type::identifier, "Expected identifier");
                parameter_name = start++->value;
            }

            parameters.emplace_back(type, parameter_name);
        } while (start->value == "," && (start++, true));
    }

    debug::assert(start++->value == ")", "Expected )");

    return ir::global::extern_function {std::move(name), std::move(parameters) };
}

ir::global::function parser::parse_function(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    auto function_prototype = parse_extern_function(start, end);

    std::vector<ir::block::block> blocks;
    blocks.emplace_back(".entry");

    while (start->value != "end") {
        if ((start + 1)->value == ":") {
            blocks.emplace_back(std::string(".") + start->value);
            start += 2;
            continue;
        }

        blocks.back().instructions.push_back(parse_instruction(start, end));
    }

    start++;

    return ir::global::function {
        std::move(function_prototype.name),
        std::move(function_prototype.parameters),
        std::move(blocks)
    };
}