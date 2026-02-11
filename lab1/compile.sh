#!/bin/bash
gcc main.c circularBuffer.c -o text

./text text Data/int_text_small.txt 1024
./text text Data/int_text_big.txt 1024

gcc main.c circularBuffer.c -o binary
./binary binary Data/test_big.dat 1024
./binary binary Data/test_small.dat 1024