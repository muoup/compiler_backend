#pragma once

namespace ir {
    struct root;
}

namespace backend {
    void analyze_ir(ir::root &root);
}