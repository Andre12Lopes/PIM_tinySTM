#!/bin/bash
echo -e "#N_THREADS\tN_TRANSACTIONS\tTIME\tN_ABORTS" > results.txt

make

for (( i = 1; i < 11; i++ )); do
	cd ../bank
	make clean
	make NR_TASKLETS=$i
	cd ../benchmark
	./launch >> results.txt
done