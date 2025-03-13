#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../utils.h"
#include "object.h"
#include "parser.h"

typedef struct {
    uint16_t *offsets;
    size_t len;
} Offsets;

Offsets orig_offset_count(const Instructions *instructions) {
    uint16_t cur_offset;
    size_t offsets_cap = 5;
    Offsets offsets = {.offsets = malloc(sizeof(uint16_t) * offsets_cap), .len = 0};
    for (size_t i = 0; i < instructions->len; i++) {
        Instruction *instr = &instructions->instructions[i];
        switch (instr->type) {
            case INSTR_ORIG:
                cur_offset = 0;
                break;
            case INSTR_END:
                if (offsets.len == offsets_cap)
                    offsets.offsets = realloc(offsets.offsets, sizeof(uint16_t) * (offsets_cap *= 2));
                offsets.offsets[offsets.len++] = cur_offset;
                break;
            case INSTR_BLKW:
                cur_offset += instr->data.u16;
                break;
            case INSTR_STRINGZ:;
                char *unescaped;
                size_t output_len;
                UnescapeResult r = unescape_string(instr->data.text, instr->data.text_len, &unescaped, &output_len);
                size_t len = (r == US_ALLOC) ? output_len : instr->data.text_len;
                if (r == US_ALLOC)
                    free(unescaped);
                cur_offset += len + 1;
                break;
            default:
                cur_offset++;
                break;
        }
    }

    // reverse array to easily dequeue items
    size_t i = 0, j = offsets.len - 1;
    while (i < j) {
        uint16_t temp = offsets.offsets[i];
        offsets.offsets[i++] = offsets.offsets[j];
        offsets.offsets[j--] = temp;
    }

    return offsets;
}

#define WRITE_HEX(i_item) fprintf(file, "%04X\n", i_item);

#define W(i_item)          \
    do {                   \
        WRITE_HEX(i_item); \
        goto break_switch; \
    } while (0)

bool write_to_object(const Instructions *instructions, char *file_name) {
    FILE *file = fopen(file_name, "w");
    if (file == NULL)
        return false;
    Offsets offsets = orig_offset_count(instructions);
    fprintf(file, "LC-3 OBJ FILE\n\n.TEXT\n");

    for (size_t i = 0; i < instructions->len; i++) {
        Instruction *instr = &instructions->instructions[i];
        union InstructionData *data = &instr->data;
        switch (instr->type) {
            case INSTR_ORIG:
                WRITE_HEX(data->u16);
                fprintf(file, "%d\n", offsets.offsets[--offsets.len]);
                break;
            case INSTR_FILL:
                W(data->u16);
            case INSTR_BLKW:
                for (uint16_t i = 0; i < data->u16; i++)
                    fprintf(file, "????\n");
                break;
            case INSTR_STRINGZ:;
                char *unescaped;
                size_t output_len;
                UnescapeResult r = unescape_string(data->text, instr->data.text_len, &unescaped, &output_len);
                const char *text = (r == US_ALLOC) ? unescaped : data->text;
                while (*text)
                    WRITE_HEX(*text++);
                WRITE_HEX(0);
                if (r == US_ALLOC)
                    free(unescaped);
                break;
            case INSTR_ADD:
                W((0b0001 << 12) | (data->dr << 9) | (data->sr1 << 6) | data->sr2);
            case INSTR_ADD_IMM:
                W((0b0001 << 12) | (data->dr << 9) | (data->sr1 << 6) < (1 << 5) | data->imm);
            case INSTR_AND:
                W((0b0101 << 12) | (data->dr << 9) | (data->sr1 << 6) | data->sr2);
            case INSTR_AND_IMM:
                W((0b0101 << 12) | (data->dr << 9) | (data->sr1 << 6) < (1 << 5) | data->imm);
            case INSTR_BR:
                W((data->br_flags.n << 11) | (data->br_flags.n << 10) | (data->br_flags.n << 9) | data->offset);
            case INSTR_JMP:
                W((0b1100 << 12) | (data->base_r << 6));
            case INSTR_JSR:
                W((0b0100 << 12) | (1 << 1) | data->offset);
            case INSTR_JSRR:
                W((0b0100 << 12) | (data->base_r << 6));
            case INSTR_LD:
                W((0b0010 << 12) | (data->dr << 9) | data->offset);
            case INSTR_LDI:
                W((0b1010 << 12) | (data->dr << 9) | data->offset);
            case INSTR_LDR:
                W((0b0110 << 12) | (data->dr << 9) | (data->base_r << 6) | data->offset);
            case INSTR_LEA:
                W((0b1110 << 12) | (data->dr << 9) | data->offset);
            case INSTR_NOT:
                W((0b1001 << 12) | (data->dr << 9) | data->offset);
            case INSTR_RTI:
                W(0b1000 << 12);
            case INSTR_ST:
                W((0b0011 << 12) | (data->sr1 << 9) | data->offset);
            case INSTR_STI:
                W((0b1011 << 12) | (data->sr1 << 9) | data->offset);
            case INSTR_STR:
                W((0b0111 << 12) | (data->sr1 << 9) | (data->base_r << 6) | data->offset);
            case INSTR_TRAP:
                W((0b1111 << 12) | (data->offset));
            case INSTR_END:
                break;
        }
    break_switch:;
    }

    free(offsets.offsets);
    fclose(file);
    return true;
}
// INSTR_ADD,
// INSTR_ADD_IMM,
// INSTR_AND,
// INSTR_AND_IMM,
// INSTR_BR,
// INSTR_JMP,
// INSTR_JSR,
// INSTR_JSRR,
// INSTR_LD,
// INSTR_LDI,
// INSTR_LDR,
// INSTR_LEA,
// INSTR_NOT,
// INSTR_RTI,
// INSTR_ST,
// INSTR_STI,
// INSTR_STR,
// INSTR_TRAP,
// INSTR_ORIG,
// INSTR_FILL,
// INSTR_BLKW,
// INSTR_STRINGZ,
// INSTR_END,
