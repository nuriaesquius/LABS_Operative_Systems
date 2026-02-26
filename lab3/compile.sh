#!/bin/bash
gcc P3_parallel.c parsePGM.c -o build/computeHistogramParallel -lpthread
build/computeHistogramParallel Data/heart.pgm Data/histogram_heart_parallel.txt 4
python3 showHistogram.py Data/histogram_heart_parallel.txt
