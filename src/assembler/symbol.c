#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "symbol.h"
#include "token.h"

void symbol_table_add_symbol(SymbolTable *table, int32_t cur_address) {
}

SymbolTableResult generate_symbol_table(SymbolTable *table, const LineTokensList *token_list, size_t *lines_read) {
    *lines_read = 0;
    int32_t next_address = -1;
    size_t symbol_cap = 5;
    table->len = 0;
    table->symbols = malloc(sizeof(*table->symbols) * symbol_cap);

    for (size_t line = 0; line < token_list->len; line++) {
        (*lines_read)++;
        LineTokens *line_tokens = &token_list->line_tokens[line];
        for (size_t i = 0; i < line_tokens->len; i++) {
            Token *token = &line_tokens->tokens[i];
            if (next_address == -1) {
                if (token->type != ORIG)
                    return ST_TOKEN_BEFORE_ORIG;
                if (i + 1 >= line_tokens->len)
                    return ST_NO_MORE_TOKENS;

                token = &line_tokens->tokens[++i];
                if (token->type != NUMBER)
                    return ST_NO_ORIG_NUMBER;
                if (token->data.number < 0)
                    return ST_NEGATIVE_ORIG;

                next_address = token->data.number;
                continue;
            }

            switch (token->type) {
                case TEXT:
                    symbol_table_add_symbol(table, next_address - 1);
                    break;
                case ORIG:
                    return ST_ORIG_INSIDE_ORIG;
                case END:
                    next_address = -1;
                    goto continue_lines;
                case BLKW:
                    token = &line_tokens->tokens[++i];
                    if (token->type != NUMBER)
                        return ST_NO_BLKW_AMOUNT;
                    if (token->data.number <= 0)
                        return ST_BAD_BLKW_AMOUNT;
                    next_address += token->data.number;
                    goto continue_lines;
                default:
                    next_address++;
                    goto continue_lines;
            }
        }
    continue_lines:;
    }

    if (next_address != -1)
        return ST_NO_END;

    return ST_SUCCESS;
}
