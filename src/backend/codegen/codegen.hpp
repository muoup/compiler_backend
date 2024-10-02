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

    static void gen_global_string(const ir::global::global_string &global_strings);
    static void gen_extern_function(const ir::global::extern_function &extern_functions);
    static void gen_function(const ir::global::function &functions);

    static void gen_instruction(const ir::block::block_instruction &instruction);
    static void gen_call(const ir::block::call &call);
    static void gen_ret(const ir::block::ret &ret);
}
