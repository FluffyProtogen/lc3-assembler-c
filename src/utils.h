#include <stddef.h>

typedef enum {
    US_NO_ALLOC,
    US_ALLOC,
    US_INVALID_ESCAPE,
} UnescapeResult;

UnescapeResult unescape_string(const char *input, size_t input_len, char **output, size_t *output_len);
