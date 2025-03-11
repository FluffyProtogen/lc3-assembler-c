#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "tokenizer.h"

int main() {
    char *lines[] = {
        "ADD R1, R0, x5432\n",
        "NOT R1, R1\n",
        "ADD R1, R1, 1\n",
        "BRnzp FLOOF\n",
    };

    for (int i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        printf("Line: %s", lines[i]);
        LineTokenizer tokenizer = {.remaining = lines[i]};

        LineTokenizerResult result;
        Token token;
        while ((result = line_tokenizer_next_token(&tokenizer, &token)) == SUCCESS) {
            debug_token_print(&token);
        }

        switch (result) {
            case SUCCESS:
            case NO_MORE_TOKENS:
                break;
            case INVALID_INTEGER:
                printf("Invalid integer at line %d\n", i + 1);
                exit(1);
                break;
            case INTEGER_TOO_LARGE:
                printf("Integer too large at line %d\n", i + 1);
                exit(1);
                break;
        }
        printf("\n");
    }
}
