#pragma once

#include <cstdint>

namespace ir {
    struct node;
    struct root;

    struct int_literal;
    struct variable;
    struct value;

    namespace block {
        struct instruction;
        struct block_instruction;
        struct block;
        struct allocate;
        struct store;
        struct load;
        struct branch;
        struct icmp;
        struct call;
        struct ret;
        struct arithmetic;

        enum icmp_type : uint8_t;
        enum arithmetic_type : uint8_t;
        enum parameter_type : uint8_t;

        const char* icmp_str(icmp_type type);
        const char* arithmetic_name(arithmetic_type type);
    }

    namespace global {
        struct global_node;

        struct parameter;
        struct global_string;
        struct extern_function;
        struct function;

        enum parameter_type : uint8_t;
    }
}