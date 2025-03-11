#!/bin/bash
gcc -g -o main src/*.c src/assembler/*.c -Wall -Wextra && ./main
