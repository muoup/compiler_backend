#include "dead_code_elim.hpp"
#include "../../ir/nodes.hpp"

#include <unordered_set>

void backend::opt::dead_code_elim(ir::root &root) {
    for (auto &fn : root.functions) {
        fn_dead_code_elim(fn);
    }
}

void backend::opt::fn_dead_code_elim(ir::global::function &fn) {
    if (fn.blocks.empty()) return;

    std::unordered_set<std::string> unreachable;

    for (auto &block : fn.blocks) {
        unreachable.insert(block.name);
    }

    // The first block is reachable by virtue of being the entry block
    unreachable.erase(fn.blocks.front().name);

    for (auto &block : fn.blocks) {
        for (auto &inst : block.instructions) {
            for (auto &reachable : inst.labels_referenced) {
                unreachable.erase(reachable);
            }
        }
    }

    const auto is_unreachable = [&unreachable](const ir::block::block &block) {
        return unreachable.contains(block.name);
    };

    erase_if(fn.blocks, is_unreachable);
}