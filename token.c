#include <stdio.h>

#include "token.h"

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
