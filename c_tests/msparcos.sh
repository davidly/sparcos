#!/bin/bash
#set -x

if [ "$1" == "" ]; then
    optlevel="3"
else
    optlevel="$1"
fi

gccpath="../gcc-14.3.0/bin"

$gccpath/sparc-linux-gcc -x c++ -DSPARCOS -DTARGET_BIG_ENDIAN -fno-builtin -I.. -O$optlevel ../sparcos.cxx ../sparc.cxx -o sparcos.elf -l:libstdc++.a -static
$gccpath/sparc-buildroot-linux-uclibc-objdump -d sparcos.elf >sparcos.txt

cp sparcos.elf /mnt/c/users/david/onedrive/sparcos/c_tests
cp sparcos.txt /mnt/c/users/david/onedrive/sparcos/c_tests
