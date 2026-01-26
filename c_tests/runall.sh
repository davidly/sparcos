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

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr tbits t tao \
           tcmp ttypes tarray trw trw2 terrno mm_old ttime fileops tpi \
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo sleeptm \
           nqueens nq1d tdir fopentst lenum trename triangle fact tld \
           tmmap termiosf;
do
    echo $arg
    for optflag in 0 1 2 3 fast;
    do
        echo test bin$optflag/$arg >>$outputfile
        $_sparcosruncmd bin$optflag/$arg.elf >>$outputfile
    done
done

for arg in esp esp7 sievesp tttsp tttusp;
do
    echo $arg
    echo test $arg >>$outputfile
    $_sparcosruncmd $arg.elf >>$outputfile
done

echo test ff
for optflag in 0 1 2 3 fast;
do
    echo test bin$optflag/ff >>$outputfile
    $_sparcosruncmd bin$optflag/ff.elf -i . ff.c >>$outputfile
done    

echo test ba
for optflag in 0 1 2 3 fast;
do
    echo test bin$optflag/ba >>$outputfile
    $_sparcosruncmd bin$optflag/ba.elf TP.BAS >>$outputfile
done    

echo test an
for optflag in 0 1 2 3 fast;
do
    echo test bin$optflag/an >>$outputfile
    $_sparcosruncmd bin$optflag/an.elf david lee >>$outputfile
done

echo running tgets with redirected stdin
for optflag in 0 1 2 3 fast;
do
    echo test bin$optflag/tgets >>$outputfile
    $_sparcosruncmd bin$optflag/tgets <tgets.txt >>$outputfile
done    

echo running targs
for optflag in 0 1 2 3 fast;
do
    echo test bin$optflag/targs a bb ccc dddd >>$outputfile
    $_sparcosruncmd bin$optflag/targs a bb ccc dddd >>$outputfile
done    

date_time=$(date)
echo "$date_time" >>$outputfile
unix2dos -f $outputfile

diff --ignore-all-space baseline_$outputfile $outputfile
