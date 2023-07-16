reset
clear

set xrange [1:8]
set xtics 2,2

#set ytics 6000, 0.1

 
set grid
set terminal jpeg font "Arial, 20"
set key bottom left Left reverse font "Arial, 14"

#set key outside
#set key at 5., 2000.

set key box linewidth 1.5
set xlabel "WiFi flows" 
set ylabel "Received packets" 
set output "recv.jpeg"

set pointsize 3

plot "11g/avg_rx_pkts.txt-final.out" using 1:2 title "802.11g" with linespoint lt 1 lw 2.5 pt 2 lc rgb "black",\
"11g/avg_rx_pkts.txt-final.out" using 1:2:5 notitle with yerrorbars lt 1 pt 2 lc rgb "black",\
"11n/avg_rx_pkts.txt-final.out" using 1:2 title "802.11n" with linespoint lt 1 lw 2.5 pt 3 lc rgb "red",\
"11n/avg_rx_pkts.txt-final.out" using 1:2:5 notitle with yerrorbars lt 1 pt 2 lc rgb "red"

