#!/bin/bash

PROTOKOLL_FILE=wr_block.prot

creates_file=./wr_bench
FILE_TYPE=gif

cat > creates_gnuplot.cmd <<EOF
set output '${creates_file}.${FILE_TYPE}'
set terminal ${FILE_TYPE}

set title 'write performance for Filesystem'
set xlabel 'number of parallel processes'
set ylabel 'bandwidth [gb/s]'

### plot 'creates.dat' with line
plot 'wr_bs.64k.dat' with lines lt 1, 'wr_bs.8m.dat' with lines lt 2,  'wr_bs.16k.dat' with lines lt 3, 'wr_bs.2m.dat' with lines lt 4
    
EOF

### grep gen_$ $PROTOKOLL_FILE | awk '{ printf (  "%d   %f\n", $1, ($1 * 10000 / $3)  ); }' > creates.dat

grep 16384$ $PROTOKOLL_FILE | awk '{ printf (  "%d   %f\n", $1, ($2 / (1024 * 1024 *1024))  ); }' > wr_bs.16k.dat
grep 65536$ $PROTOKOLL_FILE | awk '{ printf (  "%d   %f\n", $1, ($2 / (1024 * 1024 *1024))  ); }' > wr_bs.64k.dat
grep 2097152$ $PROTOKOLL_FILE | awk '{ printf (  "%d   %f\n", $1, ($2 / (1024 * 1024 *1024) ) ); }' > wr_bs.2m.dat
grep 8388608$ $PROTOKOLL_FILE | awk '{ printf (  "%d   %f\n", $1, ($2 / (1024 * 1024 *1024) ) ); }' >  wr_bs.8m.dat

gnuplot creates_gnuplot.cmd

