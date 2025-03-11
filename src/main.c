#include <stdio.h>
#include <stdlib.h>

#include "assembler/symbol.h"
#include "assembler/token.h"

int main() {
    printf("TODO: unescape strings\n");
    const char *lines[] = {
        ".orig x3000",          //
        "ADD R1, R0, x5432\n",  //
        "NoT R1, R1\n",         //
        "ADD R1, R1, 42069\n",  //
        "\n",                   //
        "; amogus\n",           //
        "BRnzp FLOOF, x1\n",    //
        ".end"                  //
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
        free_tokens_list(&token_list);
        return 1;
    }

    printf("Successfully parsed %lu lines:\n", lines_read);
    for (size_t line = 0; line < token_list.len; line++) {
        printf("LINE %lu\n", line);
        for (int i = 0; i < token_list.line_tokens[line].len; i++)
            debug_token_print(&token_list.line_tokens[line].tokens[i]);
    }

    SymbolTable table;
    SymbolTableResult st_result = generate_symbol_table(&table, &token_list, &lines_read);
    if (st_result != ST_SUCCESS) {
        printf("Failed at line %lu with err %d\n", lines_read, st_result);
        free_tokens_list(&token_list);
        free(table.symbols);
        return 1;
    }

    free_tokens_list(&token_list);
    free(table.symbols);
}
