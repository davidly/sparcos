#!/bin/bash

outputfile="test_sparcos.txt"
date_time=$(date)
echo "$date_time" >$outputfile

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
           tcmp ttypes tarray trw trw2 terrno mm_old ttime fileops tpi \
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo sleeptm \
           nqueens nq1d tdir fopentst lenum trename triangle fact tld;
do
    echo $arg
    echo test $arg >>$outputfile
    ../sparcos $arg.elf >>$outputfile
done

echo test ff
echo test ff >>$outputfile
../sparcos ff.elf -i . ff.c >>$outputfile

date_time=$(date)
echo "$date_time" >>$outputfile
unix2dos -f $outputfile

diff --ignore-all-space baseline_$outputfile $outputfile
