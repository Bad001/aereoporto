#!/bin/bash
gcc -c aereoporto.c
gcc -o aereoporto aereoporto.o
rm aereoporto.o
./aereoporto &
