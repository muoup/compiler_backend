#pragma once

#include <ostream>
#include <vector>
#include <cstdint>

#include "../../ir/node_prototypes.hpp"

namespace ir::output {
    template <typename T>
    using emitter_attachment = void(*)(std::ostream&, const T&);

    inline emitter_attachment<ir::block::block_instruction> instruction_emitter_attachment = nullptr;

    void emit(std::ostream &ostream, const ir::root& root);

    void emit_function(std::ostream &ostream, const ir::global::function &function);
    void emit_external_function(std::ostream &ostream, const ir::global::extern_function &extern_function);
    void emit_global_string(std::ostream &ostream, const ir::global::global_string &global_string);

    void emit_parameters(std::ostream &ostream, const std::vector<ir::global::parameter> &parameters);
    void emit_parameter(std::ostream &ostream, const ir::global::parameter& parameters);

    void emit_param_type(std::ostream &ostream, ir::global::parameter_type type);
}