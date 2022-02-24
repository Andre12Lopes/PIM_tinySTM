set term pdf enhanced size 7,3.2
set output "plot.pdf"
set xlabel "Thread Number" 
set ylabel "Throughput (Tx/s)"
set yrange [0:*]

plot "test_data_processed" using 1:($2/$3):4 title 'WRAM' with yerrorlines