#!/bin/bash
#set -x

if [ "$1" == "" ]; then
    echo "Usage: m.sh <sourcefile> [optlevel]"
    echo "  optlevel: 0, 1, 2 (default), 3, fast"
    exit 1
fi

if ! test -f "$1.c"; then
    echo "File $1.c not found!"
    exit 1
fi

if [ "$2" == "" ]; then
    optlevel="2"
else
    optlevel="$2"
fi

gccpath="../gcc-14.3.0/bin"
mkdir bin"$optlevel" 2>/dev/null

# use -mcpu=v7 to generate mulscc instructions for integer multiplication
$gccpath/sparc-linux-gcc -x c++ -O$optlevel -mcpu=v8 $1.c -o bin$optlevel/$1.elf -l:libstdc++.a -static

# generate s file for reference
#$gccpath/sparc-linux-gcc -x c++ -O$optlevel -S -fverbose-asm -mcpu=v8 $1.c -o bin$optlevel/$1.s

# generate disassembly
$gccpath/sparc-buildroot-linux-uclibc-objdump -d bin$optlevel/$1.elf >bin$optlevel/$1.txt

#cp $1.c /mnt/c/users/david/onedrive/sparcos/c_tests
#cp bin$optlevel/$1.elf /mnt/c/users/david/onedrive/sparcos/c_tests/bin$optlevel
#cp bin$optlevel/$1.txt /mnt/c/users/david/onedrive/sparcos/c_tests/bin$optlevel
