set term pdf enhanced size 7,3.2
set output "plot.pdf"
set xlabel "Thread Number" 
set ylabel "Throughput (Tx/s)"
set yrange [0:*]
plot "results.txt" u 1:($2/$3) notitle with linespoints