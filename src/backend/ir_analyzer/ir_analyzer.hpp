#pragma once

namespace ir {
    struct root;
}

namespace backend {
    struct analysis_root;

    void analyze_ir(ir::root &root);

    static void add_empty_metadata(ir::root &root);
}