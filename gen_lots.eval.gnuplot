#!/bin/bash


PROTOKOLL_FILE=protokoll

creates_file=./creates.pdf


cat > creates_gnuplot.cmd <<EOF
set output 'creates.gif'
set terminal gif


set title 'create () system call for Filesystem'
set xlabel 'number of parallel processes'
set ylabel 'operations per second'

### plot 'creates.dat' with line
plot 'creates.dat' with lines lt 1, 'stat.dat' with lines lt 2,  'unlink.dat' with lines  lt 3 
    
EOF

grep gen_$ $PROTOKOLL_FILE | awk '{ printf (  "%d   %f\n", $1, ($1 * 10000 / $3)  ); }' > creates.dat
grep stat_$ $PROTOKOLL_FILE | awk '{ printf (  "%d   %f\n", $1, ($1 * 10000 / $3)  ); }' > stat.dat
grep unlink_$ $PROTOKOLL_FILE | awk '{ printf (  "%d   %f\n", $1, ($1 * 10000 / $3)  ); }' > unlink.dat

gnuplot creates_gnuplot.cmd

