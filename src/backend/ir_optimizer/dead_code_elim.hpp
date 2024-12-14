#include "../../ir/node_prototypes.hpp"

#include <unordered_set>
#include <functional>

namespace backend::opt {
    using foreach_block = std::function<void(ir::block::block &)>;

    void dead_code_elim(ir::root &root);

    static void fn_dead_code_elim(ir::global::function &fn);
}