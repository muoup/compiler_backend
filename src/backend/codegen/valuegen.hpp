#pragma once

#include "registers.hpp"
#include "valuegen.hpp"

#include <string>
#include <memory>

namespace backend::codegen {
    struct function_context;

    struct vptr {
        virtual ~vptr() = default;
        virtual std::string get_address(size_t size) const = 0;
    };
    using virtual_pointer = std::unique_ptr<vptr>;

    virtual_pointer stack_allocate(backend::codegen::function_context &context, size_t size);
    virtual_pointer request_register(backend::codegen::function_context &context);

    std::string get_stack_prefix(size_t size);

    struct stack_pointer : vptr {
        size_t rsp_off;

        explicit stack_pointer(size_t rsp_off) : rsp_off(rsp_off) {}
        ~stack_pointer() override = default;

        std::string get_address(size_t size) const override {
            return get_stack_prefix(size) + " [rbp - " + std::to_string(rsp_off) + "]";
        }
    };

    struct register_storage : vptr {
        backend::codegen::register_t reg;

        explicit register_storage(backend::codegen::register_t reg) : reg(reg) {}
        ~register_storage() override = default;

        std::string get_address(size_t size) const override {
            return backend::codegen::register_as_string(reg, size);
        }
    };

    struct literal : vptr {
        std::string value;

        explicit literal(std::string value) : value(std::move(value)) {}
        ~literal() override = default;

        std::string get_address(size_t size) const override {
            return value;
        }
    };

    struct icmp_result : vptr {
        const char* flag;

        explicit icmp_result(const char* flag) : flag(flag) {}
        ~icmp_result() override = default;

        std::string get_address(size_t size) const override {
            throw std::runtime_error("ICMP result cannot be used as an address");
        }
    };
}