#!/bin/bash

FIFO=canale_torre_aereo
if [ -f "$FIFO" ]; then
	rm canale_torre_aereo
fi
gcc -c aereoporto.c
gcc -o aereoporto aereoporto.o
./aereoporto &
