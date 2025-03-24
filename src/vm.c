#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "vm.h"

void vm_randomize(VirtualMachine *vm) {
    for (size_t i = 0; i < sizeof(VirtualMachine); i++)
        ((uint8_t *)vm)[i] = rand();
}

bool vm_load(VirtualMachine *vm, char *file_name) {
    FILE *file = fopen(file_name, "r");
    if (!file)
        return false;

    fscanf(file, "LC-3 OBJ FILE\n\n.TEXT\n");
    uint16_t cur_addr, left_to_read;
    if (fscanf(file, "%hx\n%hd\n", &cur_addr, &left_to_read) != 2)
        return false;
    vm->pc = cur_addr;

    for (;;) {
        for (int i = 0; i < left_to_read; i++) {
            uint16_t result = 0;
            if (fscanf(file, "%hx\n", &result) == 1)
                vm->memory[cur_addr++] = result;
            else if (fscanf(file, "????\n%hn", &result), result == 5)
                cur_addr++;
            else
                return false;
        }

        int result = fscanf(file, "%hx\n", &cur_addr);
        if (result == EOF)
            return true;
        else if (result != 1)
            return false;

        if (fscanf(file, "%hd\n", &left_to_read) != 1)
            return false;
    }
}

uint16_t sext(uint16_t num, uint16_t bit_count) {
    if (((num >> (bit_count - 1)) & 1) == 1)
        return num | (0xFFFF << bit_count);
    else
        return num;
}

uint16_t read_reg(const VirtualMachine *vm, uint16_t reg) {
    return *(const uint16_t *[]){&vm->r0, &vm->r1, &vm->r2, &vm->r3, &vm->r4, &vm->r5, &vm->r6, &vm->r7}[reg];
}

void write_reg(VirtualMachine *vm, uint16_t reg, uint16_t value) {
    *(uint16_t *[]){&vm->r0, &vm->r1, &vm->r2, &vm->r3, &vm->r4, &vm->r5, &vm->r6, &vm->r7}[reg] = value;
    if (value == 0)
        vm->cc = CC_ZERO;
    else if (value >> 15)
        vm->cc = CC_NEGATIVE;
    else
        vm->cc = CC_POSITIVE;
}

bool vm_exec_next_instruction(VirtualMachine *vm) {
    uint16_t instr = vm->memory[vm->pc++];
    printf("%04X ", instr);
    uint16_t op_code = instr >> 12;
    uint16_t dr, sr, sr1, sr2, br, imm5, offset, value, addr;
    switch (op_code) {
        case 0x0:;  // BR
            uint16_t flags = (instr >> 9) & 0x7;
            printf("flags: %d\n", vm->cc);
            if (flags & vm->cc)
                vm->pc += sext(instr & 0x1FF, 9);
            break;
        case 0x1:  // ADD
            dr = (instr >> 9) & 0x7;
            sr1 = (instr >> 6) & 0x7;
            if ((instr >> 5) & 0x1) {
                imm5 = sext(instr & 0x1F, 5);
                uint16_t result = read_reg(vm, sr1) + imm5;
                printf("add r%d, r%d, %d = %x\n", dr, sr1, imm5, result);
                write_reg(vm, dr, result);
            } else {
                sr2 = instr & 0x7;
                uint16_t result = read_reg(vm, sr1) + read_reg(vm, sr2);
                printf("add r%d, r%d, r%d = %x\n", dr, sr1, sr2, result);
                write_reg(vm, dr, result);
            }
            break;
        case 0x2:  // LD
            dr = (instr >> 9) & 0x7;
            offset = sext(instr & 0x1FF, 9);
            addr = vm->pc + offset;
            value = vm->memory[addr];
            printf("ld (%x) = %x\n", addr, value);
            write_reg(vm, dr, value);
            break;
        case 0x3:  // ST
            sr = (instr >> 9) & 0x7;
            offset = sext(instr & 0x1FF, 9);
            addr = vm->pc + offset;
            vm->memory[addr] = read_reg(vm, sr);
            printf("st: %d\n", read_reg(vm, sr));
            break;
        case 0x4:  // JSR
            write_reg(vm, 7, vm->pc);
            if ((instr >> 11) & 0x1)
                vm->pc += sext(instr & 0x7FF, 11);
            else
                vm->pc += read_reg(vm, (instr >> 6) & 0x7);
            printf("JSR\n");
            break;
        case 0x5:  // AND
            dr = (instr >> 9) & 0x7;
            sr1 = (instr >> 6) & 0x7;
            if ((instr >> 5) & 0x1) {
                imm5 = sext(instr & 0x1F, 5);
                uint16_t result = read_reg(vm, sr1) & imm5;
                write_reg(vm, dr, result);
            } else {
                sr2 = instr & 0x7;
                uint16_t result = read_reg(vm, sr1) & read_reg(vm, sr2);
                write_reg(vm, dr, result);
            }
            printf("\n");
            break;
        case 0x6:  // LDR
            dr = (instr >> 9) & 0x7;
            br = (instr >> 6) & 0x7;
            offset = sext(instr & 0x3F, 6);
            addr = read_reg(vm, br) + offset;
            value = vm->memory[addr];
            write_reg(vm, dr, value);
            printf("ldr r%d, r%d, %d = %x (addr = %x)\n", dr, br, offset, value, addr);
            break;
        case 0x7:  // STR
            sr = (instr >> 9) & 0x7;
            br = (instr >> 6) & 0x7;
            offset = sext(instr & 0x3F, 6);
            addr = read_reg(vm, br) + offset;
            vm->memory[addr] = read_reg(vm, sr);
            printf("str r%d (%x) to %x\n", sr, read_reg(vm, sr), addr);
            break;
        case 0x8:  // RTI
            break;
        case 0x9:  // NOT
            dr = (instr >> 9) & 0x7;
            sr = (instr >> 6) & 0x7;
            write_reg(vm, dr, ~read_reg(vm, sr));
            printf("not r%d, r%d = %d\n", dr, sr, ~read_reg(vm, sr));
            break;
        case 0xA:  // LDI
            dr = (instr >> 9) & 0x7;
            offset = sext(instr & 0x1FF, 9);
            addr = vm->pc + offset;
            value = vm->memory[addr];
            write_reg(vm, dr, vm->memory[value]);
            printf("\n");
            break;
        case 0xB:  // STI
            sr = (instr >> 9) & 0x7;
            offset = sext(instr & 0x1FF, 9);
            addr = vm->pc + offset;
            value = vm->memory[addr];
            vm->memory[value] = read_reg(vm, sr);
            printf("\n");
            break;
        case 0xC:  // JMP
            vm->pc = read_reg(vm, (instr >> 6) & 0x7);
            printf("\n");
            break;
        case 0xD:  // reserved
            break;
        case 0xE:  // LEA
            dr = (instr >> 9) & 0x7;
            offset = sext(instr & 0x1FF, 9);
            *(uint16_t *[]){
                &vm->r0, &vm->r1, &vm->r2, &vm->r3, &vm->r4, &vm->r5, &vm->r6, &vm->r7,
            }[dr] = vm->pc + offset;
            printf("\n");
            break;
        case 0xF:;  // Trap
            int trap = instr & 0xFF;
            switch (trap) {
                case 0x22:;  // PUTS
                    uint16_t i = vm->r0;
                    for (;;) {
                        char c = vm->memory[i++];
                        if (!c)
                            break;
                        printf("%c", c);
                        fflush(stdout);
                    }
                    break;
                case 0x25:  // HALT
                    return false;
            }
    }
    return true;
}
