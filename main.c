#include <stdio.h>
#include <stdlib.h>

#include "token.h"

int main() {
    const char *lines[] = {
        "ADD R1, R0, x5432\n", "NoT R1, R1\n", "ADD R1, R1, 42069\n", "", "; amogus", "BRnzp FLOOF, x1\n",
    };

    LineTokens tokens;
    size_t lines_read;
    LineTokenizerResult result = tokenize_lines(&tokens, lines, sizeof(lines) / sizeof(lines[0]), &lines_read);
    if (result != SUCCESS) {
        printf("Failed at line %lu: ", lines_read);
        switch (result) {
            case INVALID_INTEGER:
                printf("invalid integer\n");
                break;
            case INTEGER_TOO_LARGE:
                printf("integer too large\n");
                break;
            default:
                break;
        }
        free(tokens.line_tokens);
        exit(1);
    }

    printf("Successfully parsed %lu lines:\n", lines_read);
    for (size_t i = 0; i < tokens.len; i++) {
        printf("line: %lu ", tokens.line_tokens[i].line);
        debug_token_print(&tokens.line_tokens[i].token);
    }

    free(tokens.line_tokens);
}
