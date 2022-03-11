set term pdf enhanced size 7,3.2
set output "plot.pdf"
set xlabel "Thread Number" 
set ylabel "Throughput (Tx/s)"
set xrange [1:*]
set yrange [0:*]

plot "results_mram_wbetl_processed" using 1:($2/$3):4 title 'wbetl' with yerrorlines, \
	 "results_mram_wtetl_processed" using 1:($2/$3):4 title 'wtetl' with yerrorlines