#pragma once

#include <ostream>
#include <vector>

namespace ir {
    struct root;

    namespace global {
        struct function;
        struct extern_function;
        struct global_string;

        struct parameter;
        enum parameter_type : uint8_t;
    }
}

namespace ir::output {
    void emit(std::ostream &ostream, const ir::root& root);

    static void emit_function(std::ostream &ostream, const ir::global::function &function);
    static void emit_external_function(std::ostream &ostream, const ir::global::extern_function &extern_function);
    static void emit_global_string(std::ostream &ostream, const ir::global::global_string &global_string);

    static void emit_parameters(std::ostream &ostream, const std::vector<ir::global::parameter> &parameters);
    static void emit_parameter(std::ostream &ostream, const ir::global::parameter& parameters);

    static void emit_param_type(std::ostream &ostream, const ir::global::parameter_type type);
}