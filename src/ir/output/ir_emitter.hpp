#pragma once

#include <ostream>
#include <vector>
#include <cstdint>
#include "../nodes.hpp"

namespace ir {
    struct root;

    namespace global {
        struct function;
        struct extern_function;
        struct global_string;

        struct parameter;
        enum parameter_type : uint8_t;
    }

    namespace block {
        struct block_instruction;
    }
}

namespace ir::output {
    template <typename T>
    using emitter_attachment = void(*)(std::ostream&, const T&);

    inline emitter_attachment<ir::block::block_instruction> instruction_emitter_attachment = nullptr;

    void emit(std::ostream &ostream, const ir::root& root);

    void emit_function(std::ostream &ostream, const ir::global::function &function);
    void emit_external_function(std::ostream &ostream, const ir::global::extern_function &extern_function);
    void emit_global_string(std::ostream &ostream, const ir::global::global_string &global_string);

    void emit_parameters(std::ostream &ostream, const std::vector<ir::variable> &params);
}