#pragma once

#include <stdbool.h>
#include "token.h"

typedef struct {
    const char *remaining;
} LineTokenizer;

typedef enum {
    SUCCESS,
    NO_MORE_TOKENS,
    INTEGER_TOO_LARGE,
    INVALID_INTEGER,
} LineTokenizerResult;

LineTokenizerResult line_tokenizer_next_token(LineTokenizer *tokenizer, Token *result);
