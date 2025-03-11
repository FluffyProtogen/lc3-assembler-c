#include <stdint.h>
#include "token.h"

typedef struct {
    struct {
        const char *symbol;
        int32_t addr;
    } *symbols;
    size_t len;
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
    ST_TRAILING_TOKENS,
    ST_ORIG_INSIDE_ORIG,
    ST_NO_END,
    ST_SYMBOL_ALREADY_EXISTS,
} SymbolTableResult;

SymbolTableResult generate_symbol_table(SymbolTable *table, const LineTokensList *line_tokens, size_t *lines_read);

void free_symbol_table(SymbolTable *table);
