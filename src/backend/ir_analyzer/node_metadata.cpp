#include "node_metadata.hpp"
#include "../../ir/nodes.hpp"
#include "scope_analyzer.hpp"

backend::instruction_metadata::instruction_metadata(const ir::block::block_instruction &instruction)
        : instruction(instruction) {}

backend::function_metadata::function_metadata(const ir::global::function &function)
        : function(function) {}