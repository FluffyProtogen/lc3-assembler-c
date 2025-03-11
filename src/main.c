#include <stdio.h>
#include <stdlib.h>

#include "assembler/symbol.h"
#include "assembler/token.h"

int main() {
    const char *lines[] = {
        ".orig x3000",          //
        "ADD R1, R0, x5432\n",  //
        "NoT R1, R1\n",         //
        "ADD R1, R1, 42069\n",  //
        "\n",                   //
        "; amogus\n",           //
        "BRnzp FLOOF, x1\n",    //
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
        free(token_list.line_tokens);
        exit(1);
    }

    printf("Successfully parsed %lu lines:\n", lines_read);
    // for (size_t line = 0; line < token_list.len; line++) {
    //     printf("LINE %lu\n", line);
    //     for (int i = 0; i < token_list.line_tokens[line].len; i++)
    //         debug_token_print(&token_list.line_tokens[line].tokens[i]);
    // }

    SymbolTable table;
    SymbolTableResult st_result = generate_symbol_table(&table, &token_list, &lines_read);
    if (st_result != ST_SUCCESS) {
        printf("Failed at line %lu: ", lines_read);
        //     return ST_TOKEN_BEFORE_ORIG;
        // if (i + 1 >= line_tokens->len)
        //     return ST_NO_MORE_TOKENS;
        switch (st_result) {
            case ST_TOKEN_BEFORE_ORIG:
                printf("Token before orig.\n");
                break;
            case ST_NO_MORE_TOKENS:
                printf("Ran out of tokens.\n");
                break;
        }
        exit(1);
    }

    // free(table.symbols);
    for (size_t i = 0; i < token_list.len; i++)
        free(token_list.line_tokens[i].tokens);
    free(token_list.line_tokens);
}
