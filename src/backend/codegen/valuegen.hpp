#pragma once

#include "registers.hpp"
#include "../../ir/nodes.hpp"

#include <string>
#include <memory>

namespace backend::codegen {
    struct function_context;
    struct register_storage;

    struct vptr {
        ir::value_size size;
        bool droppable = true;

        explicit vptr(ir::value_size size) : size(size) {}
        virtual ~vptr() = default;

        [[nodiscard]] virtual bool addressable () const { return true; }
    };
    using virtual_pointer = std::unique_ptr<vptr>;

    virtual_pointer stack_allocate(backend::codegen::function_context &context, size_t size);

    std::unique_ptr<backend::codegen::register_storage> find_register(backend::codegen::function_context &context, ir::value_size size);
    std::unique_ptr<backend::codegen::register_storage> force_find_register(backend::codegen::function_context &context, ir::value_size size);
    std::unique_ptr<backend::codegen::register_storage> claim_temp_register(backend::codegen::function_context &context, ir::value_size size);

    std::string get_stack_prefix(ir::value_size size);

    struct complex_ptr : vptr {
        int64_t offset;
        register_t reg;
        int8_t reg_scale;

        explicit complex_ptr(ir::value_size element_size, int64_t offset, register_t reg, int8_t reg_scale)
            : vptr(element_size), offset(offset), reg(reg), reg_scale(reg_scale) {}
        ~complex_ptr() override = default;
    };

    struct stack_value : vptr {
        int64_t rbp_off;

        explicit stack_value(ir::value_size element_size, int64_t rbp_off)
            : vptr(element_size), rbp_off(rbp_off) {}
        ~stack_value() override = default;
    };

    struct register_storage : vptr {
        backend::codegen::register_t reg;

        explicit register_storage(ir::value_size size, backend::codegen::register_t reg)
            : vptr(size), reg(reg) {}
        ~register_storage() override = default;
    };

    struct vptr_int_literal : vptr {
        uint64_t value;

        explicit vptr_int_literal(ir::value_size size, uint64_t value)
            : vptr(size), value(value) {}
        ~vptr_int_literal() override = default;

        [[nodiscard]] bool addressable() const override {
            return false;
        }
    };

    struct global_pointer : vptr {
        std::string name;

        explicit global_pointer(std::string name)
            : vptr(ir::value_size::ptr), name(std::move(name)) {}
        ~global_pointer() override = default;
    };

    struct icmp_result : vptr {
        ir::block::icmp_type flag;

        explicit icmp_result(ir::block::icmp_type flag)
            : vptr(ir::value_size::i1), flag(flag) {}
        ~icmp_result() override = default;
    };
}