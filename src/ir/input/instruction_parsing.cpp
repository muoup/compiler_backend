#include "instruction_parsing.hpp"
#include "../../debug/assert.hpp"

using namespace ir;

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
        assignment = parser::parse_variable(start, end);
        debug::assert(start++->value == "=", "Expected =");
    }

    debug::assert(start->type == lexer::token_type::identifier, "Expected identifier");

    auto instruction = parser::parse_unassigned_instruction(start, end);
    instruction.assigned_to = assignment;

    return instruction;
}

static ir::block::block_instruction parser::parse_unassigned_instruction(parser::lex_iter_t &start, parser::lex_iter_t end) {
    const auto &instruction = start++->value;

    if (instruction == "allocate")
        return generate_instruction<ir::block::allocate, uint8_t>(start, end);
    else if (instruction == "store")
        return generate_instruction<ir::block::store, uint8_t>(start, end);
    else if (instruction == "load")
        return generate_instruction<ir::block::load, uint8_t>(start, end);
    else if (instruction == "icmp")
        return generate_instruction<ir::block::icmp, ir::block::icmp_type>(start, end);
    else if (instruction == "branch")
        return generate_instruction<ir::block::branch, std::string, std::string>(start, end);
    else if (instruction == "iadd")
        return generate_instruction<ir::block::arithmetic>(start, end, block::arithmetic_type::iadd);
    else if (instruction == "isub")
        return generate_instruction<ir::block::arithmetic>(start, end, block::arithmetic_type::isub);
    else if (instruction == "ret")
        return generate_instruction<ir::block::ret>(start, end);
    else if (instruction == "call")
        return generate_instruction<ir::block::call, std::string>(start, end);
    else debug::assert(false, "Unknown instruction");

    throw std::runtime_error("Unreachable");
}

static uint8_t parser::parse_uint8_t(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    debug::assert(start->type == lexer::token_type::number, "Expected integer");

    return std::stoi(start++->value);
}

static std::vector<value> parser::parse_operands(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
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

static value parser::parse_value(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t end) {
    if (start->value == "%") {
        return ir::value { parse_variable(start, end) };
    } else if (start->type == lexer::token_type::number) {
        return ir::value{
            ir::int_literal{
                    static_cast<uint64_t>(std::stoi(start++->value))
            }
        };
    }

    throw std::runtime_error("Value parsing failed");
}

static variable parser::parse_variable(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t) {
    debug::assert(start++->value == "%", "Expected %");

    bool is_ptr = start->value == "ptr" && (start++, true);
    auto value = start++->value;

    return ir::variable { std::move(value), is_ptr };
}

static ir::block::icmp_type parser::parse_icmp_type(ir::parser::lex_iter_t &start, ir::parser::lex_iter_t) {
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
