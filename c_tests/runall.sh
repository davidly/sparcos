#!/bin/bash

if [ "$1" = "nested" ]; then
    _sparcosruncmd="../sparcos -h:120 ./sparcos.elf"
fi

if [ "$1" = "armos" ]; then
    _sparcosruncmd="../../ArmOS/armos -h:120 ../../ArmOS/bin/sparcos"
fi

if [ "$1" = "m68" ]; then
    _sparcosruncmd="../../m68/m68 -h:120 ../../m68/bin/sparcos"
fi

if [ "$1" = "rvos" ]; then
    _sparcosruncmd="../../rvos/rvos -h:120 ../../rvos/bin/sparcos.elf"
fi

if [ "$_sparcosruncmd" = "" ]; then
    _sparcosruncmd="../sparcos"
fi

outputfile="test_sparcos.txt"
date_time=$(date)
echo "$date_time" >$outputfile

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
           tcmp ttypes tarray trw trw2 terrno mm_old ttime fileops tpi \
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo sleeptm \
           nqueens nq1d tdir fopentst lenum trename triangle fact tld tmmap \
           esp esp7 sievesp tttsp tttusp;
do
    echo $arg
    echo test $arg >>$outputfile
    $_sparcosruncmd $arg.elf >>$outputfile
done

echo test ff
echo test ff >>$outputfile
$_sparcosruncmd ff.elf -i . ff.c >>$outputfile

echo test ba
echo test ba >>$outputfile
$_sparcosruncmd ba.elf TP.BAS >>$outputfile

echo test an
echo test an >>$outputfile
$_sparcosruncmd an.elf david lee >>$outputfile

date_time=$(date)
echo "$date_time" >>$outputfile
unix2dos -f $outputfile

diff --ignore-all-space baseline_$outputfile $outputfile
