#include "library.h"

function_builder code_unit::create_function(
    std::string name,
    ir::value_size return_type,
    std::vector<ir::variable> parameters
) {
    std::vector<ir::block::block> default_blocks;
    default_blocks.emplace_back("__entry");

    root.functions.emplace_back(name, std::move(parameters), std::move(default_blocks), return_type);

    return {
        .unit = this,
        .current_function = &root.functions.back(),
        .current_block = &root.functions.back().blocks.back()
    };
}

void function_builder::create_block(const std::string &name) {
    current_function->blocks.emplace_back(name);
    current_block = &current_function->blocks.back();
}