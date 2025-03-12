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
    char *string;
    TokenType type;
} TOKEN_STRS[] = {
    {"ADD", ADD}, {"AND", AND}, {"JSR", JSR}, {"JSRR", JSRR}, {"LD", LD},   {"LDI", LDI}, {"LDR", LDR},
    {"LEA", LEA}, {"NOT", NOT}, {"RTI", RTI}, {"ST", ST},     {"STI", STI}, {"STR", STR}, {"TRAP", TRAP},
};

const struct {
    char *string;
    TokenType type;
} PSEUDOOP_STRS[] = {{"ORIG", ORIG}, {"FILL", FILL}, {"BLKW", BLKW}, {"STRINGZ", STRINGZ}, {"END", END}};

const struct {
    char *string;
    BrFlags br_flags;
} BR_STRS[] = {
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
        return LT_INTEGER_TOO_LARGE;
    if (num > USHRT_MAX || num < SHRT_MIN)
        return LT_INTEGER_TOO_LARGE;
    if (text + cur_len != end)  // didn't fully use the number, so must be an invalid character somewhere inside it
        return LT_INVALID_INTEGER;

    *output = num;
    return LT_SUCCESS;
}

bool is_number(const char *text) {
    if (text[0] >= '0' && text[0] <= '9' || text[0] == '-')
        return true;
    if (toupper(text[0]) == 'X') {
        if (text[1] >= '0' && text[1] <= '9')
            return true;
        if (toupper(text[1]) >= 'A' && toupper(text[1]) <= 'F')
            return true;
    }
    return false;
}

LineTokenizerResult line_tokenizer_next_token(LineTokenizer *tokenizer, Token *result) {
    // first, keep eating characters until either reaching a non-whitespace or comma
    bool found = false;
    while (!found) {
        switch (tokenizer->remaining[0]) {
            case 0:
                return LT_NO_MORE_TOKENS;
            case '\n':
                return LT_NO_MORE_TOKENS;
            case ';':
                tokenizer->remaining = "";
                return LT_NO_MORE_TOKENS;
            case ',':
                *result = (Token){.span_start = tokenizer->remaining++, .span_len = 1, .type = COMMA};
                return LT_SUCCESS;
            case '"':
                *result = (Token){.span_start = tokenizer->remaining++, .span_len = 1, .type = QUOTE};
                return LT_SUCCESS;
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
           tokenizer->remaining[cur_len] != ';' && tokenizer->remaining[cur_len] != '"')
        cur_len++;

    for (size_t i = 0; i < sizeof(TOKEN_STRS) / sizeof(TOKEN_STRS[0]); i++) {
        if (strncasecmp(tokenizer->remaining, TOKEN_STRS[i].string, cur_len) == 0) {
            *result = (Token){.span_start = tokenizer->remaining, .span_len = cur_len, .type = TOKEN_STRS[i].type};
            tokenizer->remaining += cur_len;
            return LT_SUCCESS;
        }
    }

    for (size_t i = 0; i < sizeof(BR_STRS) / sizeof(BR_STRS[0]); i++) {
        if (strncasecmp(tokenizer->remaining, BR_STRS[i].string, cur_len) == 0) {
            BrFlags flags = BR_STRS[i].br_flags;
            *result = (Token){
                .span_start = tokenizer->remaining, .span_len = cur_len, .type = BR, .data = {.br_flags = flags}};
            tokenizer->remaining += cur_len;
            return LT_SUCCESS;
        }
    }

    // if text starts with ., it must be a pseudoop
    if (tokenizer->remaining[0] == '.') {
        for (size_t i = 0; i < sizeof(PSEUDOOP_STRS) / sizeof(PSEUDOOP_STRS[0]); i++) {
            if (strncasecmp(tokenizer->remaining + 1, PSEUDOOP_STRS[i].string, cur_len - 1) == 0) {
                *result =
                    (Token){.span_start = tokenizer->remaining, .span_len = cur_len, .type = PSEUDOOP_STRS[i].type};
                tokenizer->remaining += cur_len;
                return LT_SUCCESS;
            }
        }
        return LT_BAD_PSEUDOOP;
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
        return LT_SUCCESS;
    }

    // if text starts with a number, minus sign, or x, attempt to parse it and return an error if it fails
    if (is_number(tokenizer->remaining)) {
        int32_t num;
        LineTokenizerResult err;
        if ((err = parse_int(tokenizer->remaining, cur_len, &num)) != LT_SUCCESS)
            return err;

        *result = (Token){
            .span_start = tokenizer->remaining,
            .span_len = cur_len,
            .type = NUMBER,
            .data = {.number = num},
        };
        tokenizer->remaining += cur_len;

        return LT_SUCCESS;
    }

    *result = (Token){.span_start = tokenizer->remaining, .span_len = cur_len, .type = TEXT};
    tokenizer->remaining += cur_len;
    return LT_SUCCESS;
}

LineTokenizerResult tokenize_lines(LineTokensList *list, const char **lines, size_t line_count, size_t *lines_read) {
    *lines_read = 0;
    list->len = 0;
    size_t list_cap = 50;
    list->line_tokens = malloc(sizeof(LineTokens) * list_cap);
    for (size_t i = 0; i < line_count; i++) {
        (*lines_read)++;
        size_t line_tokens_cap = 5;
        LineTokens line_tokens = {.tokens = malloc(sizeof(Token) * line_tokens_cap), .line = *lines_read, .len = 0};
        LineTokenizer tokenizer = {.remaining = lines[i]};
        LineTokenizerResult result;
        Token token;
        while ((result = line_tokenizer_next_token(&tokenizer, &token)) == LT_SUCCESS) {
            if (line_tokens.len == line_tokens_cap)
                line_tokens.tokens = realloc(line_tokens.tokens, sizeof(Token) * (line_tokens_cap *= 2));
            line_tokens.tokens[line_tokens.len++] = token;
        }
        // propagate the failure up
        if (result != LT_NO_MORE_TOKENS) {
            free(line_tokens.tokens);
            return result;
        }

        if (list->len == list_cap)
            list->line_tokens = realloc(list->line_tokens, sizeof(LineTokens) * (list_cap *= 2));
        list->line_tokens[list->len++] = line_tokens;
    }
    return LT_SUCCESS;
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
        case QUOTE:
            return "QUOTE";
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
        case GETC:
            return "GETC";
        case IN:
            return "IN";
        case OUT:
            return "OUT";
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

void free_tokens_list(LineTokensList *list) {
    for (size_t i = 0; i < list->len; i++)
        free(list->line_tokens[i].tokens);
    free(list->line_tokens);
}
