#!/usr/bin/env bash
set -euo pipefail

mkdir -p bin

gcc -std=c11 -O2 -Wall -Wextra -pedantic \
  src/main.c src/circularBuffer.c \
  -o bin/myprogram
