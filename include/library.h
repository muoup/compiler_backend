#include "../src/ir/nodes.hpp"

namespace cb {
    struct function_builder;

    struct code_unit {
        ir::root root;

        code_unit() = default;

        function_builder
        create_function(std::string name, ir::value_size return_type, std::vector<ir::variable> parameters);
    };

    struct function_builder {
        code_unit *unit;
        ir::global::function *current_function;
        ir::block::block *current_block;

        void create_block(const std::string &name);

        void checkout_last_block();

        void checkout_block(const std::string &name);

        template<typename T, typename... Args>
        void create_instruction(Args &&... args) {
            current_block->instructions.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
        }
    };
}