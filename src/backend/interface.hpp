#pragma once

#include <ostream>
#include <vector>

#include "../ir/nodes.hpp"
#include "../ir/input/lexer.hpp"
#include "ir_optimizer/dead_code_elim.hpp"

namespace backend {
    std::vector<ir::lexer::token> lex(std::string_view file_name);

    ir::root gen_ast(std::string_view file_name);

    void compile(ir::root &root, std::ostream &ostream);
    void compile(std::string_view file_name, std::ostream &ostream);

    void analyze_ir(ir::root &root);
}
