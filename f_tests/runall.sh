#!/bin/bash

if [ "$1" = "nested" ]; then
    _sparcosruncmd="../sparcos -h:120 ../c_tests/sparcos.elf"
fi

if [ "$1" = "armos" ]; then
    _sparcosruncmd="../../ArmOS/armos -h:120 ../../ArmOS/bin/sparcos"
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

for arg in primes sieve e ttt mm;
do
    echo $arg
    echo test $arg >>$outputfile
    $_sparcosruncmd $arg.elf >>$outputfile
done

date_time=$(date)
echo "$date_time" >>$outputfile
unix2dos -f $outputfile

diff --ignore-all-space baseline_$outputfile $outputfile
