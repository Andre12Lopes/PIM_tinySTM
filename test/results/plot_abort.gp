set term pdf enhanced size 7,3.2
set output "plot_aborts.pdf"
set xlabel "Thread Number" 
set ylabel "Abort rate (%)"
set style fill solid 0.5
set xrange [1:*]

plot "results_mram_wbetl_processed" using 1:(($5 * 100)/($5 + $2)) title 'wbetl' with linespoints, \
	 "results_mram_wtetl_processed" using 1:(($5 * 100)/($5 + $2)) title 'wtetl' with linespoints