set term pdf enhanced size 7,3.2
set output "plot_aborts.pdf"
set xlabel "Thread Number" 
set ylabel "Abort rate (%)"
set style fill solid 0.5
set xrange [*:11]
plot "results_mram_no_ro" using 1:(($5 * 100)/($5 + $2)):6 title 'WRAM' with yerrorlines