#pragma once

#include <span>
#include "../../ir/nodes.hpp"

namespace ir {
    struct root;

    namespace global {
        struct global_string;
        struct extern_function;
    }
}

namespace codegen {
    void generate(const ir::root& root, std::ostream& out);

    void gen_global_string(const ir::global::global_string &global_strings);
    void gen_extern_function(const ir::global::extern_function &extern_functions);
    void gen_function(const ir::global::function &functions);

    void gen_instruction(const ir::block::block_instruction &instruction);
}
