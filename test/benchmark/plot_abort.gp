set term pdf enhanced size 7,3.2
set output "plot_aborts.pdf"
set xlabel "Thread Number" 
set ylabel "Abort rate (%)"
set style fill solid 0.5
set xrange [*:11]
plot "results.txt" u 1:(($4 * 100)/($4 + $2)) notitle with boxes