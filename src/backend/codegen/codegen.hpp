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

namespace backend::codegen {
    struct function_context {
        std::ostream& ostream;
        size_t rsp_off = 0;
    };

    void generate(const ir::root& root, std::ostream& ostream);

    static void gen_global_strings(std::ostream& ostream, const std::vector<ir::global::global_string> &global_strings);
    static void gen_extern_functions(std::ostream& ostream, const std::vector<ir::global::extern_function> &extern_functions);
    static void gen_defined_functions(std::ostream &ostream, const std::vector<ir::global::function> &functions);

    static void gen_function(std::ostream &ostream, const ir::global::function &function);

    static void gen_instruction(backend::codegen::function_context &context, const ir::block::block_instruction &instruction);
}
