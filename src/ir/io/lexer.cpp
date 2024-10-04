#include "lexer.hpp"

#include <optional>
#include <algorithm>
#include <unordered_set>

using namespace ir;

using lex_iter_t = std::string::const_iterator;

struct preident_token {
    lexer::token token;
    lex_iter_t end;
};

using preident_function = std::optional<preident_token>(*)(lex_iter_t, lex_iter_t);

std::optional<preident_token> get_symbol(lex_iter_t start, lex_iter_t) {
    const static std::unordered_set<char> symbols {
        '%', '=', ',', ':', '-', '(', ')'
    };

    if (!symbols.contains(*start)) return std::nullopt;

    return preident_token {
        lexer::token {
            lexer::token_type::symbol,
            std::string(start, start + 1)
        },
        start + 1
    };
}

std::optional<preident_token> get_string(lex_iter_t start, lex_iter_t end) {
    if (*start != '"') return std::nullopt;

    auto end_quote = std::string_view(start, end).find('"', 1);

    if (end_quote == std::string_view::npos) return std::nullopt;

    return preident_token {
        lexer::token {
            lexer::token_type::string,
            std::string(start + 1, start + end_quote)
        },
        start + end_quote + 1
    };
}

const static preident_function preident_functions[] = {
    get_symbol,
    get_string
};

std::vector<lexer::token> lexer::lex(const std::string &input) {
    auto iter = input.begin();
    auto unconsumed_begin = input.begin();
    const auto end = input.end();

    std::vector<lexer::token> tokens;

    const auto dump_unconsumed = [&]() {
        auto type = std::isdigit(*unconsumed_begin) ? lexer::token_type::number : lexer::token_type::identifier;

        if (unconsumed_begin != iter) {
            tokens.emplace_back(type, std::string(unconsumed_begin, iter));
            unconsumed_begin = iter;
        }
    };

    while (iter < input.end()) {
        while (std::isspace(*iter)) {
            if (unconsumed_begin != iter) dump_unconsumed();
            iter++; unconsumed_begin = iter;
        }
        if (iter == input.end()) break;

        for (auto &func : preident_functions) {
            auto token = func(iter, end);

            if (token.has_value()) {
                dump_unconsumed();
                tokens.push_back(token->token);

                iter = token->end;
                unconsumed_begin = iter;
                goto preident_found;
            }
        }

        iter++;

        preident_found:
        ;
    }

    dump_unconsumed();

    return tokens;
}