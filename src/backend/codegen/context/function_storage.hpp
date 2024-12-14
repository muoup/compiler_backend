#pragma once

#include "../codegen.hpp"
#include "../valuegen.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace backend::context {
    struct value_reference;

    inline static auto reg(int i) {
        return std::make_unique<register_storage>((register_t) i);
    }

    struct function_storage {
        function_context &parent_context;

        std::unordered_map<std::string, virtual_memory*> value_map;

        std::vector<std::string> pending_drop;
        std::vector<virtual_memory*> dropped_available;

        std::unique_ptr<register_storage> registers[register_count] = {
            reg(0), reg(1), reg(2), reg(3), reg(4),
            reg(5), reg(6), reg(7),reg(8), reg(9),
            reg(10), reg(11), reg(12), reg(13), reg(14),
        };
        std::vector<owned_vmem> misc_storage;

        void remap_value(std::string name, backend::context::virtual_memory *value);
        void map_value(std::string name, virtual_memory *value);
        void map_value(const ir::variable &var, virtual_memory *value);

        register_storage* register_ref(register_t reg);
        register_storage* get_register(register_t reg, ir::value_size size);
        void claim_temp_register(backend::context::register_t reg, context::value_reference &val);

        template <typename T, typename... Args>
        T* get_misc_storage(Args&&... args) {
            misc_storage.emplace_back(std::make_unique<T>(args...));
            return dynamic_cast<T*>(misc_storage.back().get());
        }

        void drop_ownership(std::string_view name);
        void erase_value(std::string_view name);

        void drop_reassignable();
        void erase_reassignable();

        value_reference get_value(std::string_view name);
        value_reference get_value(const ir::value &value);
        value_reference get_value(const ir::variable &var);
        value_reference get_value(const ir::int_literal &literal);

        bool has_value(std::string_view name) const;

        void ensure_in_register(value_reference &val);
    };
}