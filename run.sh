#!/bin/bash
gcc -g -o main src/main.c src/assembler/*.c -Wall -Wextra && ./main
