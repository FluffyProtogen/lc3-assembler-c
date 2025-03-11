#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "token.h"

typedef struct {
    const char *remaining;
} LineTokenizer;

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
    if ((num == LONG_MAX || num == LONG_MIN) && errno == ERANGE)
        return INTEGER_TOO_LARGE;
    if (num > USHRT_MAX || num < SHRT_MIN)
        return INTEGER_TOO_LARGE;
    if (text + cur_len != end)  // didn't fully use the number, so must be an invalid character somewhere inside it
        return INVALID_INTEGER;

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

    for (size_t i = 0; i < sizeof(TOKEN_STRINGS) / sizeof(TOKEN_STRINGS[0]); i++) {
        char *token_string = TOKEN_STRINGS[i].token_string;
        TokenType token_type = TOKEN_STRINGS[i].token_type;
        if (strncasecmp(tokenizer->remaining, token_string, cur_len) == 0) {
            *result = (Token){.span_start = tokenizer->remaining, .span_len = cur_len, .type = token_type};
            tokenizer->remaining += cur_len;
            return SUCCESS;
        }
    }

    for (size_t i = 0; i < sizeof(BR_STRINGS) / sizeof(BR_STRINGS[0]); i++) {
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

LineTokenizerResult tokenize_lines(LineTokens *line_tokens, const char **lines, size_t line_count, size_t *lines_read) {
    *lines_read = 0;
    line_tokens->len = 0;
    line_tokens->cap = 50;
    line_tokens->line_tokens = malloc(sizeof(LineToken) * line_tokens->cap);
    for (size_t i = 0; i < line_count; i++) {
        (*lines_read)++;
        LineTokenizer tokenizer = {.remaining = lines[i]};

        LineTokenizerResult result;
        Token token;
        while ((result = line_tokenizer_next_token(&tokenizer, &token)) == SUCCESS) {
            if (line_tokens->len == line_tokens->cap) {
                line_tokens->cap *= 2;
                line_tokens->line_tokens = realloc(line_tokens->line_tokens, sizeof(LineToken) * line_tokens->cap);
            }
            line_tokens->line_tokens[line_tokens->len++] = (LineToken){.token = token, .line = *lines_read};
        }

        switch (result) {
            case SUCCESS:
            case NO_MORE_TOKENS:
                break;
            case INVALID_INTEGER:
                return INVALID_INTEGER;
            case INTEGER_TOO_LARGE:
                return INTEGER_TOO_LARGE;
        }
    }
    return SUCCESS;
}

char *token_type_string(TokenType token_type) {
    switch (token_type) {
        case ADD:
            return "ADD";
        case AND:
            return "AND";
        case BR:
            return "BR";
        case JMP:
            return "JMP";
        case JSR:
            return "JSR";
        case JSRR:
            return "JSRR";
        case LD:
            return "LD";
        case LDI:
            return "LDI";
        case LDR:
            return "LDR";
        case LEA:
            return "LEA";
        case NOT:
            return "NOT";
        case RTI:
            return "RTI";
        case ST:
            return "ST";
        case STI:
            return "STI";
        case STR:
            return "STR";
        case TRAP:
            return "TRAP";
        case ORIG:
            return "ORIG";
        case FILL:
            return "FILL";
        case BLKW:
            return "BLKW";
        case STRINGZ:
            return "STRINGZ";
        case END:
            return "END";
        case COMMA:
            return "COMMA";
        case TEXT:
            return "TEXT";
        case PUTS:
            return "PUTS";
        case HALT:
            return "HALT";
        case RET:
            return "RET";
        case NUMBER:
            return "NUMBER";
        case REGISTER:
            return "REGISTER";
    }
    return "UNREACHABLE";
}

void print_extra_data(const Token *token) {
    switch (token->type) {
        case BR:
            if (token->data.br_flags.n)
                printf("n");
            if (token->data.br_flags.z)
                printf("z");
            if (token->data.br_flags.p)
                printf("p");
            break;
        case REGISTER:
            printf("%d", token->data.reg);
            break;
        case NUMBER:
            printf("%d", token->data.number);
            break;
        default:
            break;
    }
}

void debug_token_print(const Token *token) {
    printf("type: ");
    char *type = token_type_string(token->type);
    printf("%-9s extra: ", type);
    print_extra_data(token);

    printf(" span: %.*s\n", (int)token->span_len, token->span_start);
}
