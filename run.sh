#!/bin/bash
gcc -g3 -lm -o main src/*.c src/assembler/*.c -Wall -Wextra -fsanitize=address,undefined -fno-omit-frame-pointer && ./main
