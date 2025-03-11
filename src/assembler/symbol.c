#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "symbol.h"
#include "token.h"

#define RETURN_FAIL_IF_TRAILING_TOKENS() \
    if (line_tokens->len >= i + 1) {     \
        return ST_TRAILING_TOKENS;       \
    }

SymbolTableResult generate_symbol_table(SymbolTable *table, const LineTokensList *token_list, size_t *lines_read) {
    *lines_read = 0;
    int32_t cur_address = -1;
    size_t symbol_cap = 5;
    table->len = 0;
    table->symbols = malloc(sizeof(*table->symbols) * symbol_cap);

    for (size_t line = 0; line < token_list->len; line++) {
        (*lines_read)++;
        LineTokens *line_tokens = &token_list->line_tokens[line];
        for (size_t i = 0; i < line_tokens->len; i++) {
            Token *token = &line_tokens->tokens[i];
            if (cur_address == -1) {
                if (token->type != ORIG)
                    return ST_TOKEN_BEFORE_ORIG;
                if (i + 1 >= line_tokens->len)
                    return ST_NO_MORE_TOKENS;

                token = &line_tokens->tokens[++i];
                if (token->type != NUMBER)
                    return ST_NO_ORIG_NUMBER;
                if (token->data.number < 0)
                    return ST_NEGATIVE_ORIG;

                cur_address = token->data.number;
                RETURN_FAIL_IF_TRAILING_TOKENS();
            }
        }
    }

    return ST_SUCCESS;
}
