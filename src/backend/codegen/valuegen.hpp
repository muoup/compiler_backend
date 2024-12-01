#pragma once

#include "registers.hpp"
#include "../../ir/nodes.hpp"

#include <string>
#include <memory>

namespace backend::context {
    struct function_context;
    struct register_storage;
    struct memory_addr;

    struct virtual_memory {
        ir::value_size size;

        explicit virtual_memory(ir::value_size size) : size(size) {}
        virtual ~virtual_memory() = default;

        [[nodiscard]] virtual bool addressable () const { return true; }
    };
    using owned_vmem = std::unique_ptr<virtual_memory>;

    memory_addr* stack_allocate(backend::context::function_context &context, size_t size);

    register_storage * find_register(backend::context::function_context &context, ir::value_size size);
    register_storage * force_find_register(backend::context::function_context &context, ir::value_size size);

    std::string get_stack_prefix(ir::value_size size);

    struct memory_addr : virtual_memory {
        struct scaled_reg {
            register_t reg;
            int8_t scale;
        };

        int64_t offset;

        std::optional<scaled_reg> scaled;
        std::optional<register_t> unscaled;

        memory_addr(ir::value_size element_size, int64_t offset, scaled_reg scaled, register_t unscaled)
            : virtual_memory(element_size), offset(offset), scaled(scaled), unscaled(unscaled) {}
        memory_addr(ir::value_size element_size, int64_t offset, register_t unscaled)
                : virtual_memory(element_size), offset(offset), unscaled(unscaled) {}
        memory_addr(ir::value_size element_size, int64_t offset, scaled_reg scaled)
            : virtual_memory(element_size), offset(offset), scaled(scaled) {}
        memory_addr(ir::value_size element_size, int64_t rbp_off)
            : virtual_memory(element_size), offset(rbp_off), unscaled(register_t::rbp) {}
        ~memory_addr() override = default;
    };

    struct register_storage : virtual_memory {
        backend::context::register_t reg;
        std::string owner;
        bool tampered = false;

        // do not allow register to be moved out of
        bool frozen = false;

        explicit register_storage(backend::context::register_t reg)
            : virtual_memory(ir::value_size::none), reg(reg) {}
        ~register_storage() override = default;

        [[nodiscard]] bool in_use() const {
            return !owner.empty();
        }

        void grab(ir::value_size size) {
            this->size = size;
            this->tampered = true;
            this->frozen = true;
        }

        void unclaim() {
            this->owner.clear();
            this->frozen = false;
        }
    };

    struct vptr_int_literal : virtual_memory {
        uint64_t value;

        explicit vptr_int_literal(ir::value_size size, uint64_t value)
            : virtual_memory(size), value(value) {}
        ~vptr_int_literal() override = default;

        [[nodiscard]] bool addressable() const override {
            return false;
        }
    };

    struct global_pointer : virtual_memory {
        std::string name;

        explicit global_pointer(std::string name)
            : virtual_memory(ir::value_size::ptr), name(std::move(name)) {}
        ~global_pointer() override = default;
    };

    struct icmp_result : virtual_memory {
        ir::block::icmp_type flag;

        explicit icmp_result(ir::block::icmp_type flag)
            : virtual_memory(ir::value_size::i1), flag(flag) {}
        ~icmp_result() override = default;
    };
}