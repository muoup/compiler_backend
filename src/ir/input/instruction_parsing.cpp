#include <iostream>
#include "element_parsers.hpp"

#include "../../debug/assert.hpp"

using namespace ir;

using lex_iter_t = parser::lex_iter_t;

template <typename InstructionType, typename... Misc>
auto generate_instruction(parser::lex_iter_t &start, parser::lex_iter_t end) {
    std::unique_ptr<InstructionType> instruction = std::make_unique<InstructionType>(parser::parse_argument<Misc>(start, end)...);
    auto values = parser::parse_operands(start, end);

    return ir::block::block_instruction { std::move(instruction), std::move(values) };
}

template <typename InstructionType, typename... Args>
auto generate_instruction(parser::lex_iter_t &start, parser::lex_iter_t end, Args... args) {
    return ir::block::block_instruction {
        std::make_unique<InstructionType>(args...),
        parser::parse_operands(start, end)
    };
}

ir::block::block_instruction parser::parse_instruction(parser::lex_iter_t &start, parser::lex_iter_t end) {
    std::optional<ir::variable> assignment {};

    if (start->value == "%") {
        assignment = parser::parse_variable(start, end, ir::value_size::none);
        debug::assert(start++->value == "=", "Expected =");
    }

    auto instruction = parser::parse_unassigned_instruction(start, end);
    instruction.assigned_to = assignment;

    return instruction;
}

ir::block::block_instruction parser::parse_unassigned_instruction(parser::lex_iter_t &start, parser::lex_iter_t end) {
    if (auto size = maybe_value_size(start, end); size.has_value()) {
        debug::assert(start->type == lexer::token_type::number, "Expected integer");

        return block::block_instruction {
            std::make_unique<block::literal>(
                    ir::int_literal {
                                *size,
                                static_cast<uint64_t>(std::stoi(start++->value))
                        }
                    ),
            {}
        };
    }

    debug::assert(start->type == lexer::token_type::identifier, "Expected identifier");

    const auto &instruction = start++->value;

    if (instruction == "allocate")
        return generate_instruction<ir::block::allocate, uint8_t>(start, end);
    else if (instruction == "store")
        return generate_instruction<ir::block::store, uint8_t>(start, end);
    else if (instruction == "load")
        return generate_instruction<ir::block::load, value_size>(start, end);
    else if (instruction == "icmp")
        return generate_instruction<ir::block::icmp, ir::block::icmp_type>(start, end);
    else if (instruction == "branch")
        return generate_instruction<ir::block::branch, std::string, std::string>(start, end);
    else if (instruction == "jmp")
        return generate_instruction<ir::block::jmp, std::string>(start, end);
    else if (instruction == "add")
        return generate_instruction<ir::block::arithmetic>(start, end, block::arithmetic_type::add);
    else if (instruction == "sub")
        return generate_instruction<ir::block::arithmetic>(start, end, block::arithmetic_type::sub);
    else if (instruction == "mul")
        return generate_instruction<ir::block::arithmetic>(start, end, block::arithmetic_type::mul);
    // TODO: idiv and irem
    else if (instruction == "ret")
        return generate_instruction<ir::block::ret>(start, end);
    else if (instruction == "call")
        return generate_instruction<ir::block::call, std::string, value_size>(start, end);
    else if (instruction == "phi")
        return generate_instruction<ir::block::phi, std::string, std::string>(start, end);
    else if (instruction == "select")
        return generate_instruction<ir::block::select>(start, end);
    else if (instruction == "zext")
        return generate_instruction<ir::block::zext, value_size>(start, end);
    else if (instruction == "sext")
        return generate_instruction<ir::block::sext, value_size>(start, end);
    else debug::assert(false, (std::string("Unknown instruction: ") + instruction).c_str());

    throw std::runtime_error("Unreachable");
}

uint8_t parser::parse_uint8_t(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    debug::assert(start->type == lexer::token_type::number, "Expected integer");

    return std::stoi(start++->value);
}

std::vector<value> parser::parse_operands(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    std::vector<value> operands {};

    if (start->type == lexer::token_type::break_line)
        return operands;

    start--;

    do {
        start++;
        operands.push_back(parse_value(start, end));
    } while (start->value == ",");

    return operands;
}

std::optional<ir::value_size> parser::maybe_value_size(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {\
    std::string_view size = start->value;

    if (size == "void") return value_size::none;
    if (size == "i8") return value_size::i8;
    if (size == "i1") return value_size::i1;
    if (size == "i16") return value_size::i16;
    if (size == "i32") return value_size::i32;
    if (size == "i64") return value_size::i64;
    if (size == "ptr") return value_size::ptr;

    return std::nullopt;
}

value_size parser::parse_value_size(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    if (auto size = maybe_value_size(start, end); size.has_value()) {
        start++;
        return *size;
    }

    std::cout << "WARNING: No value size specified, this will not be supported in the future!" << '\n';
    std::cout << "Found " << start->value << '\n';
    std::cout << "Defaulting to i32" << '\n';

    return value_size::i32;
}

variable parser::parse_variable(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end, ir::value_size size) {
    debug::assert(start++->value == "%", "Expected %");

    return variable { size, start++->value };
}

value parser::parse_value(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    auto size = parse_value_size(start, end);

    if (start->value == "%") {
        start++;
        return ir::value {
            ir::variable { size, start++->value }
        };
    } else if (start->type == lexer::token_type::number) {
        return ir::value{
            ir::int_literal {
                size,
                static_cast<uint64_t>(std::stoi(start++->value))
            }
        };
    }

    throw std::runtime_error("Value parsing failed");
}

ir::block::icmp_type parser::parse_icmp_type(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t) {
    const auto &op = start++->value;

    if      (op == "eq") return ir::block::icmp_type::eq;
    else if (op == "ne") return ir::block::icmp_type::neq;
    else if (op == "slt") return ir::block::icmp_type::slt;
    else if (op == "sle") return ir::block::icmp_type::sle;
    else if (op == "sgt") return ir::block::icmp_type::sgt;
    else if (op == "sge") return ir::block::icmp_type::sge;
    else if (op == "ult") return ir::block::icmp_type::ult;
    else if (op == "ule") return ir::block::icmp_type::ule;
    else if (op == "ugt") return ir::block::icmp_type::ugt;
    else if (op == "uge") return ir::block::icmp_type::uge;
    else debug::assert(false, "Unknown icmp type");

    throw std::runtime_error("Unreachable");
}
