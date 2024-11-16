#pragma once

#include <string>
#include <vector>

namespace ir::lexer {
    enum class token_type {
        identifier,
        number,
        string,
        symbol,
        break_line
    };

    struct token {
        token_type type;
        std::string value;
    };

    std::vector<token> lex(std::string_view input);
}