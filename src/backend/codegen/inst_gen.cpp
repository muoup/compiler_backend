#include "inst_gen.hpp"

#include "context/value_reference.hpp"

using namespace backend;

void codegen::gen_lea(context::function_context &context, const context::virtual_memory *dest, uint64_t m,
                      const context::virtual_memory *x, uint64_t b) {
    context.add_asm_node<as::inst::lea>(
        as::create_operand(dest),
        as::create_operand(context::memory_addr {
            ir::value_size::none,
            (int64_t) b,
            context::memory_addr::scaled_reg {
                dynamic_cast<const context::register_storage *>(x)->reg,
                (int8_t) m
            }
        })
    );
}

void codegen::gen_lea(context::function_context &context,
                      const context::virtual_memory *dest,
                      uint64_t m, const context::value_reference& x,
                      uint64_t b) {
    if (auto lit = x.get_literal()) {
        b += m * lit->value;

        context.add_asm_node<as::inst::mov>(
            as::create_operand(dest),
            as::create_operand(ir::int_literal { ir::value_size::i64, b })
        );
        return;
    }

    gen_lea(context, dest, m, *x.get_vmem(), b);
}

void codegen::gen_lea(context::function_context &context, const context::virtual_memory *dest,
                      const context::value_reference &scaled_reg, uint8_t scale, uint64_t offset,
                      const context::value_reference &unscaled_reg) {
    if (auto lit = unscaled_reg.get_literal()) {
        offset += lit->value;
        gen_lea(context, dest, offset, scaled_reg, scale);
        return;
    }

    if (auto lit = scaled_reg.get_literal()) {
        offset += lit->value * scale;

        context.add_asm_node<as::inst::lea>(
            as::create_operand(dest),
            as::create_operand(context::memory_addr {
                ir::value_size::none,
                (int64_t) offset,
                *unscaled_reg.get_register()
            })
        );
        return;
    }

    context.add_asm_node<as::inst::lea>(
        as::create_operand(dest),
        as::create_operand(context::memory_addr {
            ir::value_size::none,
            (int64_t) offset,
            context::memory_addr::scaled_reg {
                *scaled_reg.get_register(),
                (int8_t) scale
            },
            *unscaled_reg.get_register()
        })
    );
}