#!/bin/bash
gcc main.c circularBuffer.c splitCommand.c -o shell

./shell < input.txt 