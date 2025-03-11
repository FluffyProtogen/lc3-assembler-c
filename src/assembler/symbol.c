#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "symbol.h"
#include "token.h"

SymbolTableResult add_symbol(SymbolTable *table, size_t *table_cap, const char *symbol, int32_t cur_address) {
    for (size_t i = 0; i < table->len; i++) {
        if (strcasecmp(table->symbols[i].symbol, symbol) == 0)
            return ST_SYMBOL_ALREADY_EXISTS;
    }

    if (table->len == *table_cap)
        table->symbols = realloc(table->symbols, sizeof(*table->symbols) * (*table_cap *= 2));
    table->symbols[table->len].symbol = symbol;
    table->symbols[table->len++].addr = cur_address;

    return ST_SUCCESS;
}

SymbolTableResult generate_symbol_table(SymbolTable *table, const LineTokensList *token_list, size_t *lines_read) {
    *lines_read = 0;
    int32_t next_address = -1;
    size_t table_cap = 5;
    table->len = 0;
    table->symbols = malloc(sizeof(*table->symbols) * table_cap);

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
                case TEXT:;
                    char *symbol = strndup(token->span_start, token->span_len);
                    if (add_symbol(table, &table_cap, symbol, next_address) != ST_SUCCESS) {
                        free(symbol);
                        return ST_SYMBOL_ALREADY_EXISTS;
                    }
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

void free_symbol_table(SymbolTable *table) {
    for (size_t i = 0; i < table->len; i++)
        free(table->symbols[i].symbol);
    free(table->symbols);
}
