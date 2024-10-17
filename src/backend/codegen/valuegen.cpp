#include "valuegen.hpp"

#include "codegen.hpp"

#include <sstream>

std::string backend::codegen::stack_allocate(backend::codegen::function_context &context, size_t size) {
    // TODO: This will not work when arrays are implemented, as the offset will need to change depending on the index accessed
    context.rsp_off += size;

    std::stringstream ss;
    ss << "[rbp - " << context.rsp_off << "]";

    return ss.str();
}