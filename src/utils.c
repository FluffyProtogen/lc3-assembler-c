#include "utils.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

UnescapeResult unescape_string(const char *input, size_t input_len, char **output, size_t *output_len) {
    bool allocated = false;
    size_t len = 0;
    for (size_t i = 0; i < input_len; i++) {
        if (input[len++] == '\\') {
            allocated = true;
            switch (input[++i]) {
                case 'n':
                case '\\':
                    break;
                default:
                    return US_INVALID_ESCAPE;
            }
        }
    }

    if (!allocated)
        return US_NO_ALLOC;

    *output = malloc(len + 1);
    *output_len = len;
    len = 0;
    for (size_t i = 0; i < input_len; i++) {
        if (input[len] == '\\') {
            allocated = true;
            char escape;
            switch (input[++i]) {
                case 'n':
                    escape = '\n';
                    break;
                case '\\':
                    escape = '\\';
                    break;
                default:
                    printf("Unreachable unescape");
                    exit(1);
            }
            (*output)[len++] = escape;
        } else
            (*output)[len++] = input[i];
    }
    (*output)[len] = 0;

    return US_ALLOC;
}

bool fit_to_bits(int32_t number, uint8_t bits, uint16_t *result) {
    int32_t min = -pow(2, bits - 1);
    int32_t max = pow(2, bits - 1) - 1;
    if (number < min || number > max)
        return false;
    *result = ((uint16_t)number) & (0xFFFF >> (16 - bits));
    return true;
}
