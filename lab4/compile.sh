#!/bin/bash
gcc main.c parsePGM.c -o computeHistogram -lpthread
./computeHistogram Data/heart.pgm Data/histogram.txt 1 1 4
python3 showHistogram.py Data/histo.txt
