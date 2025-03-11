#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>

#include "token.h"
#include "tokenizer.h"

const struct {
    char *token_string;
    TokenType token_type;
} TOKEN_STRINGS[] = {
    {"ADD", ADD}, {"AND", AND}, {"JSR", JSR}, {"JSRR", JSRR}, {"LD", LD},   {"LDI", LDI}, {"LDR", LDR},
    {"LEA", LEA}, {"NOT", NOT}, {"RTI", RTI}, {"ST", ST},     {"STI", STI}, {"STR", STR}, {"TRAP", TRAP},
};

const struct {
    char *br_string;
    BrFlags br_flags;
} BR_STRINGS[] = {
    {"BR", {.n = true, .z = true, .p = true}},
    {"BRN", {.n = true}},
    {"BRZ", {.z = true}},
    {"BRP", {.p = true}},
    {"BRNZ", {.n = true, .z = true}},
    {"BRNP", {.n = true, .p = true}},
    {"BRZP", {.z = true, .p = true}},
    {"BRNZP", {.n = true, .z = true, .p = true}},
};

LineTokenizerResult parse_int(const char *text, size_t cur_len, int32_t *output) {
    errno = 0;
    char *end;
    long num = strtol(text + (text[0] == 'x'), &end, (text[0] == 'x') ? 16 : 10);
    if ((num == LONG_MAX || num == LONG_MIN) && errno == ERANGE) {
        return INTEGER_TOO_LARGE;
    }
    // didn't fully use the number, so must be an invalid character somewhere inside it
    if (text + cur_len != end) {
        return INVALID_INTEGER;
    }

    *output = num;
    return SUCCESS;
}

LineTokenizerResult line_tokenizer_next_token(LineTokenizer *tokenizer, Token *result) {
    // first, keep eating characters until either reaching a non-whitespace or comma
    bool found = false;
    while (!found) {
        switch (tokenizer->remaining[0]) {
            case 0:
                return NO_MORE_TOKENS;
            case '\n':
                return NO_MORE_TOKENS;
            case ';':
                tokenizer->remaining = "";
                return NO_MORE_TOKENS;
            case ',':
                *result = (Token){.span_start = tokenizer->remaining++, .span_len = 1, .type = COMMA};
                return SUCCESS;
            case ' ':
                tokenizer->remaining++;
                break;
            default:
                found = true;
                break;
        }
    }

    // find where the current token ends
    int cur_len = 0;
    while (tokenizer->remaining[cur_len] != ' ' && tokenizer->remaining[cur_len] != 0 &&
           tokenizer->remaining[cur_len] != ',' && tokenizer->remaining[cur_len] != '\n' &&
           tokenizer->remaining[cur_len] != ';')
        cur_len++;

    for (int i = 0; i < sizeof(TOKEN_STRINGS) / sizeof(TOKEN_STRINGS[0]); i++) {
        char *token_string = TOKEN_STRINGS[i].token_string;
        TokenType token_type = TOKEN_STRINGS[i].token_type;
        if (strncasecmp(tokenizer->remaining, token_string, cur_len) == 0) {
            *result = (Token){.span_start = tokenizer->remaining, .span_len = cur_len, .type = token_type};
            tokenizer->remaining += cur_len;
            return SUCCESS;
        }
    }

    for (int i = 0; i < sizeof(BR_STRINGS) / sizeof(BR_STRINGS[0]); i++) {
        if (strncasecmp(tokenizer->remaining, BR_STRINGS[i].br_string, cur_len) == 0) {
            BrFlags flags = BR_STRINGS[i].br_flags;
            *result = (Token){
                .span_start = tokenizer->remaining,
                .span_len = cur_len,
                .type = BR,
                .data = {.br_flags = flags},
            };
            tokenizer->remaining += cur_len;
            return SUCCESS;
        }
    }

    // if text starts with an R and is of length
    if (cur_len == 2 && toupper(tokenizer->remaining[0]) == 'R' &&
        (tokenizer->remaining[1] >= '0' && tokenizer->remaining[1] <= '9')) {
        char reg = tokenizer->remaining[1] - '0';
        *result = (Token){
            .span_start = tokenizer->remaining,
            .span_len = cur_len,
            .type = REGISTER,
            .data = {.reg = reg},
        };
        tokenizer->remaining += cur_len;
        return SUCCESS;
    }

    // if text starts with a number, minus sign, or x, attempt to parse it and return an error if it fails
    if (((tokenizer->remaining[0] >= '0' && tokenizer->remaining[0] <= '9') || tokenizer->remaining[0] == '-') ||
        (tokenizer->remaining[0] == 'x' && (tokenizer->remaining[1] >= '0' && tokenizer->remaining[1] <= '9'))) {
        int32_t num;
        LineTokenizerResult err;
        if ((err = parse_int(tokenizer->remaining, cur_len, &num)) != SUCCESS)
            return err;

        *result = (Token){
            .span_start = tokenizer->remaining,
            .span_len = cur_len,
            .type = NUMBER,
            .data = {.number = num},
        };
        tokenizer->remaining += cur_len;

        return SUCCESS;
    }

    *result = (Token){.span_start = tokenizer->remaining, .span_len = cur_len, .type = TEXT};
    tokenizer->remaining += cur_len;
    return SUCCESS;
}
