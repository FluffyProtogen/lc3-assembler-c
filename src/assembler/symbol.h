#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "token.h"

typedef struct {
    struct {
        char *symbol;
        int32_t addr;
    } *symbols;
    size_t sym_len;
    struct {
        int32_t orig_addr;
        int32_t end_addr;
    } *addr_spans;
    size_t addr_len;
} SymbolTable;

typedef enum {
    ST_SUCCESS,
    ST_NO_MORE_TOKENS,
    ST_ORIG_NO_START_ADDR,
    ST_NEGATIVE_ORIG,
    ST_NO_ORIG_NUMBER,
    ST_TOKEN_BEFORE_ORIG,
    ST_OVERLAPPING_MEM,
    ST_NO_BLKW_AMOUNT,
    ST_BAD_BLKW_AMOUNT,
    ST_BAD_STRINGZ,
    ST_BAD_STRING_ESCAPE,
    ST_ORIG_INSIDE_ORIG,
    ST_NO_END,
    ST_SYMBOL_ALREADY_EXISTS,
} SymbolTableResult;

SymbolTableResult generate_symbol_table(SymbolTable *table, const LineTokensList *line_tokens, size_t *lines_read);

void free_symbol_table(SymbolTable *table);

bool symbol_table_get(const SymbolTable *table, const char *span_start, size_t span_len, int32_t *output);
