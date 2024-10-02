#pragma once

#include <string>
#include <vector>

namespace ir::lexer {
    enum class token_type {
        identifier,
        number,
        string,
        symbol,
    };

    struct token {
        token_type type;
        std::string value;
    };

    std::vector<token> lex(const std::string& input);
}