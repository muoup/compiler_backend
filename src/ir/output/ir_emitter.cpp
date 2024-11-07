#include <iomanip>
#include "ir_emitter.hpp"

#include "../nodes.hpp"

void ir::output::emit(std::ostream &ostream, const ir::root &root) {
    for (const auto &global_string : root.global_strings)
        emit_global_string(ostream, global_string);

    for (const auto &extern_function : root.extern_functions)
        emit_external_function(ostream, extern_function);

    for (const auto &function : root.functions)
        emit_function(ostream, function);
}

void ir::output::emit_global_string(std::ostream &ostream, const ir::global::global_string &global_string) {
    ostream
        << "global_string %"
        << global_string.name
        << " = "
        << "\"" << global_string.value << "\"\n";
}

void ir::output::emit_external_function(std::ostream &ostream, const ir::global::extern_function &extern_function) {
    ostream
        << "extern fn void "
        << extern_function.name;

    emit_parameters(ostream, extern_function.parameters);
}

void ir::output::emit_function(std::ostream &ostream, const ir::global::function &function) {
    ostream
        << "define fn " << value_size_str(function.return_type) << " "
        << function.name;

    emit_parameters(ostream, function.parameters);

    ostream << "\n";

    for (const auto &block : function.blocks) {
        if (block.instructions.empty())
            continue;

        ostream << "." << block.name << ":\n";

        for (const auto &inst : block.instructions) {
            if (ir::output::instruction_emitter_attachment) {
                std::stringstream ss;
                inst.print(ss);

                ostream << std::left << std::setw(40) << ss.str();

                ir::output::instruction_emitter_attachment(ostream, inst);

                ostream << std::endl;
            } else {
                inst.print(ostream);
                ostream << '\n';
            }
        }
    }

    ostream << "end";
}

void ir::output::emit_parameters(std::ostream &ostream, const std::vector<ir::variable> &params) {
    ostream << "(";

    for (size_t i = 0; i < params.size(); i++) {
        params[i].print(ostream);

        if (i + 1 < params.size())
            ostream << ", ";
    }

    ostream << ")";
}