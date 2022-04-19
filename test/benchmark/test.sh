#!/bin/bash
echo -e "N_THREADS\tN_TRANSACTIONS\tTIME\tN_ABORTS\tPROCESS_TIME\tCOMMIT_TIME\tWASTED_TIME" > results.txt

make

for (( i = 1; i < 21; i++ )); do
	cd ../bank
	make clean
	make NR_TASKLETS=$i
	cd ../benchmark
	
	for (( j = 0; j < 30; j++ )); do
		./launch >> results.txt
	done
done