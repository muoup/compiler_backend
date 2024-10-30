#pragma once

#include "registers.hpp"
#include "../../ir/nodes.hpp"

#include <string>
#include <memory>

namespace backend::codegen {
    struct function_context;
    struct register_storage;

    struct vptr {
        bool droppable = true;

        virtual ~vptr() = default;

        [[nodiscard]] virtual bool addressable () const { return true; }
        [[nodiscard]] virtual std::string get_address(size_t size) const = 0;
        [[nodiscard]] virtual size_t get_size() const { return 8; }
    };
    using virtual_pointer = std::unique_ptr<vptr>;

    virtual_pointer stack_allocate(backend::codegen::function_context &context, size_t size);

    std::unique_ptr<backend::codegen::register_storage> find_register(backend::codegen::function_context &context);
    std::unique_ptr<backend::codegen::register_storage> force_find_register(backend::codegen::function_context &context);

    std::string get_stack_prefix(size_t size);

    struct stack_pointer : vptr {
        size_t rsp_off;
        size_t alloc_size;

        explicit stack_pointer(size_t rsp_off, size_t alloc_size) : rsp_off(rsp_off), alloc_size(alloc_size) {}
        ~stack_pointer() override = default;

        [[nodiscard]] std::string get_address(size_t size) const override {
            return get_stack_prefix(size) + " [rbp - " + std::to_string(rsp_off) + "]";
        }
        [[nodiscard]] size_t get_size() const override {
            return rsp_off;
        }
    };

    struct register_storage : vptr {
        backend::codegen::register_t reg;

        explicit register_storage(backend::codegen::register_t reg, size_t size = 8) : reg(reg) {}
        ~register_storage() override = default;

        [[nodiscard]] std::string get_address(size_t size) const override {
            return backend::codegen::register_as_string(reg, size);
        }
        [[nodiscard]] size_t get_size() const override {
            return 8;
        }
    };

    struct literal : vptr {
        uint64_t value;

        explicit literal(uint64_t value) : value(value) {}
        ~literal() override = default;

        [[nodiscard]] bool addressable() const override {
            return false;
        }
        [[nodiscard]] std::string get_address(size_t size) const override {
            return std::to_string(value);
        }
    };

    struct global_pointer : vptr {
        std::string name;

        explicit global_pointer(std::string name) : name(std::move(name)) {}
        ~global_pointer() override = default;

        [[nodiscard]] std::string get_address(size_t size) const override {
            return name;
        }
    };

    struct icmp_result : vptr {
        ir::block::icmp_type flag;

        explicit icmp_result(ir::block::icmp_type flag) : flag(flag) {}
        ~icmp_result() override = default;

        [[nodiscard]] std::string get_address(size_t size) const override {
            throw std::runtime_error("ICMP result cannot be used as an address");
        }
        [[nodiscard]] size_t get_size() const override {
            throw std::runtime_error("ICMP result does not have a size");
        }
    };
}