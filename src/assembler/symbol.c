#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../utils.h"
#include "symbol.h"
#include "token.h"

#define ADVANCE_TOKEN                      \
    do {                                   \
        if (i + 1 >= line_tokens->len)     \
            return ST_NO_MORE_TOKENS;      \
        token = &line_tokens->tokens[++i]; \
    } while (0)

SymbolTableResult add_symbol(SymbolTable *table, size_t *table_cap, char *symbol, int32_t cur_address) {
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
                ADVANCE_TOKEN;
                if (token->type != NUMBER)
                    return ST_NO_ORIG_NUMBER;
                if (token->data.number < 0)
                    return ST_NEGATIVE_ORIG;

                next_address = token->data.number;
                goto continue_lines;
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
                case STRINGZ:
                    ADVANCE_TOKEN;
                    if (token->type != QUOTE)
                        return ST_BAD_STRINGZ;
                    ADVANCE_TOKEN;
                    if (token->type != TEXT)
                        return ST_BAD_STRINGZ;

                    char *unescaped;
                    size_t output_len;
                    UnescapeResult result =
                        unescape_string(token->span_start, token->span_len, &unescaped, &output_len);
                    if (result == US_INVALID_ESCAPE)
                        return ST_BAD_STRING_ESCAPE;
                    size_t len = (result == US_ALLOC) ? output_len : token->span_len;
                    next_address += len + 1;  // + 1 from null terminator
                    if (result == US_ALLOC)
                        free(unescaped);

                    ADVANCE_TOKEN;
                    if (token->type != QUOTE)
                        return ST_BAD_STRINGZ;
                    goto continue_lines;
                case BLKW:
                    ADVANCE_TOKEN;
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

bool symbol_table_get(const SymbolTable *table, const char *span_start, size_t span_len, int32_t *output) {
    for (size_t i = 0; i < table->len; i++) {
        if (strncasecmp(table->symbols[i].symbol, span_start, span_len) == 0) {
            *output = table->symbols[i].addr;
            return true;
        }
    }
    return false;
}
