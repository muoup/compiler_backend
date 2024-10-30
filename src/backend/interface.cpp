#include "interface.hpp"

#include "ir_analyzer/ir_analyzer.hpp"
#include "codegen/codegen.hpp"

void backend::compile(ir::root &root, std::ostream &ostream) {
    analyze_ir(root);
    backend::codegen::generate(root, ostream);
}

void backend::analyze_ir(ir::root &root) {
    backend::md::analyze(root);
}