#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "assembler/object.h"
#include "assembler/parser.h"
#include "assembler/symbol.h"
#include "assembler/token.h"

int main() {
    int ret = 0;
    const char *lines[] = {
        ".orig x3000",  //
        "LEA R1, X10",  //
        ".end",         //
    };

    LineTokensList token_list;
    size_t lines_read;
    LineTokenizerResult lt_result = tokenize_lines(&token_list, lines, sizeof(lines) / sizeof(lines[0]), &lines_read);
    if (lt_result != LT_SUCCESS) {
        printf("Failed at line %lu: ", lines_read);
        switch (lt_result) {
            case LT_INVALID_INTEGER:
                printf("invalid integer\n");
                break;
            case LT_INTEGER_TOO_LARGE:
                printf("integer too large\n");
                break;
            case LT_BAD_PSEUDOOP:
                printf("bad pseudoop\n");
                break;
            default:
                break;
        }
        ret = 1;
        goto free_tokens;
    }

    printf("Successfully parsed %lu lines:\n", lines_read);
    for (size_t line = 0; line < token_list.len; line++) {
        printf("LINE %lu\n", line);
        for (size_t i = 0; i < token_list.line_tokens[line].len; i++)
            debug_token_print(&token_list.line_tokens[line].tokens[i]);
    }

    SymbolTable symbol_table;
    SymbolTableResult st_result = generate_symbol_table(&symbol_table, &token_list, &lines_read);
    if (st_result != ST_SUCCESS) {
        printf("Symbol table failed at line %lu with err %d\n", lines_read, st_result);
        ret = 1;
        goto free_symbols;
    }

    for (size_t i = 0; i < symbol_table.sym_len; i++)
        printf("symbol: %s  addr: %x\n", symbol_table.symbols[i].symbol, symbol_table.symbols[i].addr);

    Instructions instructions;
    ParserResult ps_result = parse_instructions(&instructions, &token_list, &symbol_table, &lines_read);
    if (ps_result != PS_SUCCESS) {
        printf("Parsing failed at line %lu with err %d: %s\n", lines_read, ps_result, lines[lines_read - 1]);
        ret = 1;
        goto free_instructions;
    }

    printf("\n--Instructions len: %lu--\n", instructions.len);
    for (size_t i = 0; i < instructions.len; i++)
        printf("instruction: %d\n", instructions.instructions[i].type);
    write_to_object(&instructions, "amogus.obj");

free_instructions:
    free(instructions.instructions);
free_symbols:
    free_symbol_table(&symbol_table);
free_tokens:
    free_tokens_list(&token_list);
    return ret;
}
