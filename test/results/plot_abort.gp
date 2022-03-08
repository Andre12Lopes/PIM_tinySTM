set term pdf enhanced size 7,3.2
set output "plot_aborts.pdf"
set xlabel "Thread Number" 
set ylabel "Abort rate (%)"
set style fill solid 0.5
set xrange [1:*]

plot "results_wram_no_ro_processed" using 1:(($5 * 100)/($5 + $2)) title 'WRAM NO RO' with linespoints, \
	 "results_mram_no_ro_processed" using 1:(($5 * 100)/($5 + $2)) title 'MRAM NO RO' with linespoints, \
     "results_mram_ro_processed" using 1:(($5 * 100)/($5 + $2)) title 'MRAM W/ RO' with linespoints