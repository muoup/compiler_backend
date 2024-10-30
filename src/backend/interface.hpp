#pragma once

#include <ostream>
#include "../ir/node_prototypes.hpp"

namespace backend {
    void compile(ir::root &root, std::ostream &ostream);

    void analyze_ir(ir::root &root);
}
