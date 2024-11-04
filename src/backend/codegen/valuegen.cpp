#include "valuegen.hpp"
#include "codegen.hpp"

#include "../ir_analyzer/node_metadata.hpp"
#include "dataflow.hpp"

#include <sstream>

std::unique_ptr<backend::codegen::register_storage>
backend::codegen::force_find_register(backend::codegen::function_context &context, ir::value_size size) {
    if (auto find = backend::codegen::find_register(context, size); find)
        return find;

    backend::codegen::empty_register(context, backend::codegen::register_t::rax);
    return std::make_unique<backend::codegen::register_storage>(size, backend::codegen::register_t::rax);
}

backend::codegen::virtual_pointer backend::codegen::stack_allocate(backend::codegen::function_context &context, ir::value_size size) {
    context.current_stack_size += ir::size_in_bytes(size);

    return std::make_unique<stack_value>(size, context.current_stack_size);
}

std::unique_ptr<backend::codegen::register_storage>
backend::codegen::find_register(backend::codegen::function_context &context, ir::value_size size) {
    // First check if any registers are being dropped, the most recent dropped registers are going
    // to be the operands dropped in the current instruction, so a separate routine for defaulting to
    // those is not needed.
    if (context.dropped_reassignable) {
        auto reassign = context.dropped_available.back();
        context.dropped_available.pop_back();

        return std::make_unique<backend::codegen::register_storage>(size, reassign);
    }

    // Otherwise check to see if any registers can be taken temporarily
    // i = 1 as rax should not be tampered with
    for (size_t i = 1; i < register_count; i++) {
        if (context.register_mem[i] || context.register_is_param[i]) continue;

        context.register_tampered[i] = true;
        return std::make_unique<backend::codegen::register_storage>(size, (register_t) i);
    }

    return nullptr;
}

std::string backend::codegen::get_stack_prefix(ir::value_size size) {
    switch (ir::size_in_bytes(size)) {
        case 1:
            return "BYTE";
        case 2:
            return "WORD";
        case 4:
            return "DWORD";
        case 8:
            return "QWORD";
    }

    throw std::runtime_error("unsupported size type");
}