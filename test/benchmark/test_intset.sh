#!/bin/bash
echo -e "N_THREADS\tN_TRANSACTIONS\tTIME\tN_ABORTS" > results_intset.txt

make

for (( i = 1; i < 12; i++ )); do
	cd ../intset
	make clean
	make NR_TASKLETS=$i
	cd ../benchmark
	
	for (( j = 0; j < 10; j++ )); do
		./launch_intset >> results_intset.txt
	done
done