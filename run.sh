#!/bin/bash
gcc -g -lm -o main src/*.c src/assembler/*.c -Wall -Wextra -fsanitize=address && ./main
