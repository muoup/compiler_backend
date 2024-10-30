#pragma once

#include <string>
#include <cstdint>

namespace backend {
    enum size_type : uint8_t {
        BYTE = 1,
        WORD = 2,
        DWORD = 4,
        QWORD = 8
    };

    /**
     * A value is anything which can produce a string representation for use in an instruction
     * (e.g. an immediate, a global string reference, a variable, etc)
     */
    struct value {
        virtual std::string generate() = 0;
    };

    /**
     * An rvalue represents a logical value being held, but not a location in stack memory it resides
     */
    struct rvalue : value {};

    /**
     * An lvalue represents a location in which stack memory can be stored, and may already be stored
     */
    struct lvalue : value {};

    struct int_immediate : rvalue {
        uint64_t data;

        explicit int_immediate(uint64_t data) : data(data) {}

        std::string generate() override {
            return std::to_string(data);
        }
    };

    struct global_string : rvalue {
        std::string name;

        explicit global_string(std::string name) : name(std::move(name)) {}

        std::string generate() override {
            return name;
        }
    };

    struct memory_stored : lvalue {
        size_type size;
        int memory_offset;

        memory_stored(size_type size, int memory_offset) : size(size), memory_offset(memory_offset) {}
    };
}
