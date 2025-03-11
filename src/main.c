#include <stdio.h>
#include <stdlib.h>

#include "assembler/parser.h"
#include "assembler/symbol.h"
#include "assembler/token.h"

int main() {
    printf("TODO: check for overlapping memory from origs\n");
    int ret = 0;
    const char *lines[] = {
        ".orig x3000",             //
        "ADD R1, R0, -16\n",       //
        "NoT R1, R1\n",            //
        "ADD R1, R1, 42069\n",     //
        "\n",                      //
        "; amogus\n",              //
        "FLUF BRnzp FLOOF, x1\n",  //
        ".blkw 3",                 //
        "FLOOF ADD\n",             //
        ".end",                    //
        ".orig x4000",             //
        "AMOgus ADD R1, R1, R1",   //
        "owo .stringz \"hi\\n\"",
        "Fluffy MantledBeast LEA R2, 0\n",
        ".end",
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

    for (size_t i = 0; i < symbol_table.len; i++)
        printf("symbol: %s  addr: %x\n", symbol_table.symbols[i].symbol, symbol_table.symbols[i].addr);

    Instructions instructions;
    ParserResult ps_result = parse_instructions(&instructions, &token_list, &symbol_table, &lines_read);
    if (ps_result != PS_SUCCESS) {
        printf("Parsing failed at line %lu with err %d\n", lines_read, ps_result);
        ret = 1;
        goto free_instructions;
    }

free_instructions:
    free(instructions.instructions);
free_symbols:
    free_symbol_table(&symbol_table);
free_tokens:
    free_tokens_list(&token_list);
    return ret;
}
