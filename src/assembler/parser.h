#pragma once

#include <stdint.h>
#include "symbol.h"
#include "token.h"

typedef enum {
    INSTR_ADD,
    INSTR_ADD_IMM,
    INSTR_AND,
    INSTR_AND_IMM,
    INSTR_BR,
    INSTR_JMP,
    INSTR_JSR,
    INSTR_JSRR,
    INSTR_LD,
    INSTR_LDI,
    INSTR_LDR,
    INSTR_LEA,
    INSTR_NOT,
    INSTR_RTI,
    INSTR_ST,
    INSTR_STI,
    INSTR_STR,
    INSTR_TRAP,
    INSTR_ORIG,
    INSTR_FILL,
    INSTR_BLKW,
    INSTR_STRINGZ,
    INSTR_END,
} InstructionType;

// has the lifetime of the text used to create it
typedef struct {
    union {
        struct {
            uint8_t dr;
            uint8_t sr1;
            uint8_t sr2;
            uint8_t base_r;
            BrFlags br_flags;
            union {
                uint16_t offset;
                uint16_t imm;
            };
        };
        uint8_t reg;
        uint16_t u16;
        struct {
            const char *text;
            size_t text_len;
        };
    } data;
    InstructionType type;
} Instruction;

typedef struct {
    Instruction *instructions;
    size_t len;
} Instructions;

typedef enum {
    PS_SUCCESS,
    PS_TOKEN_BEFORE_ORIG,
    PS_NO_MORE_TOKENS,
    PS_BAD_TOKEN,
    PS_NEGATIVE_ORIG,
    PS_BAD_BLKW,
    PS_BAD_STRING_ESCAPE,
    PS_TRAILING_TOKENS,
    PS_NUMBER_TOO_LARGE,
    PS_SYMBOL_NOT_PRESENT,
    PS_OVERFLOWING_ADDR,
} ParserResult;

ParserResult parse_instructions(Instructions *instructions,
                                const LineTokensList *line_tokens,
                                const SymbolTable *symbol_table,
                                size_t *lines_read);
