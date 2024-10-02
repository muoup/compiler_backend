#pragma once

#include "nodes.hpp"

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <variant>
#include <optional>

namespace ir {
    namespace global {
        struct global_node;
    }

    struct node {
    };

    struct literal {
        uint64_t value;

        explicit literal(uint64_t value) : value(value) {}
    };

    struct variable {
        bool is_pointer;
        std::string name;

        explicit variable(std::string name, bool is_pointer) : name(std::move(name)), is_pointer(is_pointer) {}
    };

    struct value {
        std::variant<literal, variable> val;

        explicit value(literal val) : val(val) {}
        explicit value(variable val) : val(std::move(val)) {}
    };

    namespace block {
        struct instruction : node {};

        struct block_instruction : node {
            std::optional<variable> assignment;
            std::unique_ptr<instruction> inst;

            explicit block_instruction(std::optional<variable> assignment, std::unique_ptr<instruction> instruction)
                : assignment(std::move(assignment)), inst(std::move(instruction)) {}
        };

        struct block : node {
            std::string name;
            std::vector<block_instruction> instructions {};

            explicit block(std::string name) : name(std::move(name)), instructions() {};
        };

        struct allocate : instruction {
            size_t allocation_size;
            size_t alignment;

            allocate(size_t allocation_size, size_t alignment)
                : allocation_size(allocation_size), alignment(alignment) {}
        };

        struct store : instruction {
            uint8_t size;
            value val;
            variable dest;

            store(value value, variable dest, uint8_t size)
                    : val(std::move(value)), dest(std::move(dest)) {}
        };

        struct load : instruction {
            uint8_t size;
            variable val;

            explicit load(variable val, uint8_t size)
                : val(std::move(val)), size(size) {}
        };

        enum icmp_type : uint8_t {
            eq, neq,
            slt, sgt, slte, sgte,
            ult, ugt, ulte, ugte
        };

        struct icmp : instruction {
            icmp_type type;
            value lhs;
            value rhs;

            icmp(icmp_type type, value lhs, value rhs)
                : type(type), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        };

        struct call : instruction {
            std::string name;
            std::vector<value> args {};

            explicit call(std::string name, std::vector<value> args)
                    : name(std::move(name)), args(std::move(args)) {}
        };

        struct ret : instruction {
            std::optional<value> val;

            explicit ret(std::optional<value> val)
                : val(std::move(val)) {}
        };
    }

    namespace global {
        struct global_node : node {};

        enum parameter_type {
            i8, i16, i32, i64, ptr
        };

        struct parameter {
            parameter_type type;
            std::string name;
        };

        struct global_string : global_node {
            std::string name;
            std::string value;

            explicit global_string(std::string name, std::string value)
                : name(std::move(name)), value(std::move(value)) {}
        };

        struct extern_function : global_node {
            std::string name;
            std::vector<parameter> parameters;

            explicit extern_function(std::string name, std::vector<parameter> parameters)
                : name(std::move(name)), parameters(std::move(parameters)) {}
        };

        struct function : global_node {
            std::string name;
            std::vector<parameter> parameters;
            std::vector<block::block> blocks;

            explicit function(std::string name, std::vector<parameter> parameters, std::vector<block::block> blocks)
                : name(std::move(name)), parameters(std::move(parameters)), blocks(std::move(blocks)) {}
        };
    }

    struct root : node {
        std::vector<global::global_string> global_strings;
        std::vector<global::extern_function> extern_functions;
        std::vector<global::function> functions;

        root() = default;
    };
}