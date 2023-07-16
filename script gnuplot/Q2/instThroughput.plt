reset
clear

set xrange [0:8]


set grid
set terminal jpeg font "arial, 20"
set key top right Left reverse samplen 4 width 2 font "arial, 14"

set key box linewidth 1.5
set xlabel "Time[s]" 
set ylabel "Instant Throughput[Mbps]" 
set output "instantThroughput.jpeg"

set pointsize 2

plot "instant_T1.txt" using 1:2 title "without disturb" with linespoint lt 1 lw 2.5 pt 2 lc rgb "red",\
"instant_T2.txt" using 1:2 title "with disturb" with linespoint lt 1 lw 2.5 pt 2 lc rgb "blue"



