#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    US_NO_ALLOC,
    US_ALLOC,
    US_INVALID_ESCAPE,
} UnescapeResult;

UnescapeResult unescape_string(const char *input, size_t input_len, char **output, size_t *output_len);

// returns false if number doesn't fit
bool fit_to_bits(int32_t number, uint8_t bits, uint16_t *result);
