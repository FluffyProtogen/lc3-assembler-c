#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    ADD,
    AND,
    BR,
    JMP,
    JSR,
    JSRR,
    LD,
    LDI,
    LDR,
    LEA,
    NOT,
    RTI,
    ST,
    STI,
    STR,
    TRAP,
    ORIG,
    FILL,
    BLKW,
    STRINGZ,
    END,
    COMMA,
    TEXT,
    PUTS,
    HALT,
    RET,
    NUMBER,
    REGISTER,
} TokenType;

typedef struct {
    bool n : 1;
    bool z : 1;
    bool p : 1;
} BrFlags;

typedef struct {
    const char *span_start;
    size_t span_len;
    union {
        BrFlags br_flags;
        uint8_t reg;
        int32_t number;
    } data;
    TokenType type;
} Token;

typedef struct {
    Token token;
    size_t line;
} LineTokensList;

typedef struct {
    LineTokensList *line_tokens;
    size_t len;
    size_t cap;
} LineTokenList;

typedef enum {
    LT_SUCCESS,
    LT_NO_MORE_TOKENS,
    LT_INTEGER_TOO_LARGE,
    LT_INVALID_INTEGER,
} LineTokenizerResult;

LineTokenizerResult tokenize_lines(LineTokenList *list, const char **lines, size_t line_count, size_t *lines_read);

void debug_token_print(const Token *token);
