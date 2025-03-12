#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../utils.h"
#include "parser.h"
#include "symbol.h"
#include "token.h"

#define ADVANCE_TOKEN                      \
    do {                                   \
        if (i + 1 >= line_tokens->len)     \
            return PS_NO_MORE_TOKENS;      \
        token = &line_tokens->tokens[++i]; \
    } while (0)

#define EXPECT_TOKEN(t_type)               \
    do {                                   \
        if (i + 1 >= line_tokens->len)     \
            return PS_NO_MORE_TOKENS;      \
        token = &line_tokens->tokens[++i]; \
        if (token->type != t_type)         \
            return PS_BAD_TOKEN;           \
    } while (0)

#define EXPECT_CALC_OFFSET(bits)                                                                   \
    do {                                                                                           \
        if (i + 1 >= line_tokens->len)                                                             \
            return PS_NO_MORE_TOKENS;                                                              \
        token = &line_tokens->tokens[++i];                                                         \
        if (token->type == NUMBER)                                                                 \
            calc_offset = token->data.number;                                                      \
        else if (token->type == TEXT) {                                                            \
            if (!symbol_table_get(symbol_table, token->span_start, token->span_len, &calc_offset)) \
                return PS_SYMBOL_NOT_PRESENT;                                                      \
            calc_offset -= next_address;                                                           \
        } else                                                                                     \
            return PS_BAD_TOKEN;                                                                   \
        if (!fit_to_bits(calc_offset, bits, &temp_instr.data.offset))                              \
            return PS_NUMBER_TOO_LARGE;                                                            \
    } while (0)

void add_instruction(Instructions *instrs, size_t *instrs_cap, Instruction instr) {
    if (instrs->len == *instrs_cap)
        instrs->instructions = realloc(instrs->instructions, sizeof(Instruction) * (*instrs_cap *= 2));
    instrs->instructions[instrs->len++] = instr;
}

#define PUSH_CONTINUE(i_instr)                         \
    do {                                               \
        add_instruction(instrs, &instrs_cap, i_instr); \
        goto continue_lines;                           \
    } while (0)

ParserResult parse_instructions(Instructions *instrs,
                                const LineTokensList *token_list,
                                const SymbolTable *symbol_table,
                                size_t *lines_read) {
    *lines_read = 0;
    int32_t next_address = -1;
    size_t instrs_cap = 50;
    instrs->len = 0;
    instrs->instructions = malloc(sizeof(Instruction) * instrs_cap);

    for (size_t line = 0; line < token_list->len; line++) {
        (*lines_read)++;
        LineTokens *line_tokens = &token_list->line_tokens[line];
        size_t i;
        for (i = 0; i < line_tokens->len; i++) {
            Token *token = &line_tokens->tokens[i];
            if (next_address == -1) {
                if (token->type != ORIG)
                    return PS_TOKEN_BEFORE_ORIG;
                EXPECT_TOKEN(NUMBER);
                if (token->data.number < 0)
                    return PS_NEGATIVE_ORIG;
                next_address = token->data.number;
                if (i + 1 < line_tokens->len)
                    return PS_TRAILING_TOKENS;
                PUSH_CONTINUE(((Instruction){.type = INSTR_ORIG, .data.u16 = token->data.number}));
            }

            switch (token->type) {
                case TEXT:
                case BLKW:
                case STRINGZ:
                case ORIG:
                case END:
                    break;
                default:
                    next_address++;
            }

            Instruction temp_instr;
            int32_t calc_offset;
            switch (token->type) {
                case TEXT:
                    continue;
                case BLKW:
                    EXPECT_TOKEN(NUMBER);
                    if (token->data.number <= 0)
                        return PS_BAD_BLKW;
                    next_address += token->data.number;
                    PUSH_CONTINUE(((Instruction){.type = INSTR_BLKW, .data = {.u16 = token->data.number}}));
                case STRINGZ:
                    EXPECT_TOKEN(QUOTE);
                    EXPECT_TOKEN(TEXT);
                    char *unescaped;
                    size_t output_len;
                    UnescapeResult result =
                        unescape_string(token->span_start, token->span_len, &unescaped, &output_len);
                    if (result == US_INVALID_ESCAPE)
                        return PS_BAD_STRING_ESCAPE;
                    size_t len = (result == US_ALLOC) ? output_len : token->span_len;
                    next_address += len + 1;  // + 1 from null terminator
                    if (result == US_ALLOC)
                        free(unescaped);
                    EXPECT_TOKEN(QUOTE);
                    PUSH_CONTINUE(((Instruction){.type = INSTR_STRINGZ,
                                                 .data = {.text = token->span_start, .text_len = token->span_len}}));
                case END:
                    next_address = -1;
                    PUSH_CONTINUE(((Instruction){.type = INSTR_END}));
                case ADD:
                    temp_instr = (Instruction){};
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.dr = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.sr1 = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    ADVANCE_TOKEN;
                    if (token->type == REGISTER) {
                        temp_instr.type = INSTR_ADD;
                        temp_instr.data.sr2 = token->data.reg;
                    } else if (token->type == NUMBER) {
                        temp_instr.type = INSTR_ADD_IMM;
                        if (!fit_to_bits(token->data.number, 5, &temp_instr.data.imm))
                            return PS_NUMBER_TOO_LARGE;
                    } else {
                        return PS_BAD_TOKEN;
                    }
                    PUSH_CONTINUE(temp_instr);
                case AND:
                    temp_instr = (Instruction){};
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.dr = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.sr1 = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    ADVANCE_TOKEN;
                    if (token->type == REGISTER) {
                        temp_instr.type = INSTR_AND;
                        temp_instr.data.sr2 = token->data.reg;
                    } else if (token->type == NUMBER) {
                        temp_instr.type = INSTR_AND_IMM;
                        if (!fit_to_bits(token->data.number, 5, &temp_instr.data.imm))
                            return PS_NUMBER_TOO_LARGE;
                    } else {
                        return PS_BAD_TOKEN;
                    }
                    PUSH_CONTINUE(temp_instr);
                case BR:
                    temp_instr = (Instruction){.type = INSTR_BR, .data.br_flags = token->data.br_flags};
                    EXPECT_CALC_OFFSET(9);
                    PUSH_CONTINUE(temp_instr);
                case JMP:
                    EXPECT_TOKEN(REGISTER);
                    PUSH_CONTINUE(((Instruction){.type = INSTR_JMP, .data.reg = token->data.reg}));
                case JSR:
                    temp_instr = (Instruction){.type = INSTR_JSR};
                    EXPECT_CALC_OFFSET(11);
                    PUSH_CONTINUE(temp_instr);
                case JSRR:
                    EXPECT_TOKEN(REGISTER);
                    PUSH_CONTINUE(((Instruction){.type = INSTR_JSRR, .data.base_r = token->data.reg}));
                case LD:
                    temp_instr = (Instruction){.type = INSTR_LD};
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.dr = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_CALC_OFFSET(9);
                    PUSH_CONTINUE(temp_instr);
                case LDI:
                    temp_instr = (Instruction){.type = INSTR_LDI};
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.dr = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_CALC_OFFSET(9);
                    PUSH_CONTINUE(temp_instr);
                case LDR:
                    temp_instr = (Instruction){.type = INSTR_LDR};
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.dr = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.base_r = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_CALC_OFFSET(6);
                    PUSH_CONTINUE(temp_instr);
                case LEA:
                    temp_instr = (Instruction){.type = INSTR_LEA};
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.dr = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_CALC_OFFSET(9);
                    PUSH_CONTINUE(temp_instr);
                case NOT:
                    temp_instr = (Instruction){.type = INSTR_NOT};
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.dr = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.sr1 = token->data.reg;
                    PUSH_CONTINUE(temp_instr);
                case RET:
                    PUSH_CONTINUE(((Instruction){.type = INSTR_JMP, .data.reg = 7}));
                case RTI:
                    PUSH_CONTINUE(((Instruction){.type = INSTR_RTI}));
                case ST:
                    temp_instr = (Instruction){.type = INSTR_ST};
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.sr1 = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_CALC_OFFSET(9);
                    PUSH_CONTINUE(temp_instr);
                case STI:
                    temp_instr = (Instruction){.type = INSTR_STI};
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.sr1 = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_CALC_OFFSET(9);
                    PUSH_CONTINUE(temp_instr);
                case STR:
                    temp_instr = (Instruction){.type = INSTR_STR};
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.sr1 = token->data.reg;
                    EXPECT_TOKEN(COMMA);
                    EXPECT_TOKEN(REGISTER);
                    temp_instr.data.dr = token->data.reg;
                    EXPECT_CALC_OFFSET(6);
                    PUSH_CONTINUE(temp_instr);
                case TRAP:
                    EXPECT_TOKEN(NUMBER);
                    if (token->data.number < 0 || token->data.number > (1 << 8))
                        return PS_NUMBER_TOO_LARGE;
                    PUSH_CONTINUE(((Instruction){.type = INSTR_TRAP, .data.u16 = token->data.number}));
                case GETC:
                    PUSH_CONTINUE(((Instruction){.type = INSTR_TRAP, .data.u16 = 0x20}));
                case OUT:
                    PUSH_CONTINUE(((Instruction){.type = INSTR_TRAP, .data.u16 = 0x21}));
                case PUTS:
                    PUSH_CONTINUE(((Instruction){.type = INSTR_TRAP, .data.u16 = 0x22}));
                case IN:
                    PUSH_CONTINUE(((Instruction){.type = INSTR_TRAP, .data.u16 = 0x23}));
                case HALT:
                    PUSH_CONTINUE(((Instruction){.type = INSTR_TRAP, .data.u16 = 0x25}));
            }
        }
    continue_lines:
        if (i + 1 < line_tokens->len)
            return PS_TRAILING_TOKENS;
    }

    return PS_SUCCESS;
}
