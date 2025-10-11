#!/bin/bash
#set -x

if [ "$1" == "" ]; then
    optflags="2"
else
    optflags="$1"
fi

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
           tcmp ttypes tarray trw trw2 terrno mm_old ttime fileops tpi \
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo sleeptm \
           nqueens nq1d tdir  lenum tex trename triangle fact tld ff \
           an ba fopentst tmmap;
do
    echo $arg
    for optflag in 0 1 2 3 fast;
    do
        mkdir bin"$optflag" 2>/dev/null

        if [ "$optflag" != "fast" ]; then
            m.sh $arg $optflag &
        else    
            m.sh $arg $optflag
        fi
    done
done

for arg in esp esp7 sievesp tttsp tttusp;
do
    echo $arg
    ma.sh $arg
done

msparcos.sh

echo "Waiting for all processes to complete..."
wait
