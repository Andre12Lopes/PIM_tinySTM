set term pdf enhanced size 7,3.2
set output "plot.pdf"
set xlabel "Thread Number" 
set ylabel "Throughput (Tx/s)"
set xrange [1:*]
set yrange [0:*]

plot "results_wram_no_ro_processed" using 1:($2/$3):4 title 'WRAM NO RO' with yerrorlines, \
	 "results_mram_no_ro_processed" using 1:($2/$3):4 title 'MRAM NO RO' with yerrorlines, \
     "results_mram_ro_processed" using 1:($2/$3):4 title 'MRAM W/ RO' with yerrorlines