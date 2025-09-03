#!/bin/bash
#set -x

if [ "$1" == "" ]; then
    echo "Usage: m.sh <sourcefile> [optlevel]"
    echo "  optlevel: 0, 1, 2 (default), 3, fast"
    exit 1
fi

if [ "$2" == "" ]; then
    optlevel="2"
else
    optlevel="$2"
fi

gccpath="../gcc-14.3.0/bin"

$gccpath/sparc-linux-gfortran -O$optlevel $1.f90 -o $1.elf -static
$gccpath/sparc-buildroot-linux-uclibc-objdump -d $1.elf >$1.txt

cp $1.f90 /mnt/c/users/david/onedrive/sparcos/f_tests
cp $1.elf /mnt/c/users/david/onedrive/sparcos/f_tests
cp $1.txt /mnt/c/users/david/onedrive/sparcos/f_tests
