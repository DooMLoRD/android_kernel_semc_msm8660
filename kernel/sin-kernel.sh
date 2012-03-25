#!/bin/bash

die () {
    echo >&2 "$@"
    exit 1
}

[ "$1" = "" ] && die "fsconfig.xml has to be specified as first parameter"
[ "$2" = "" ] && die "ramdisk.img has to be specified as second parameter"

MKELFPY=mkelf.py
type -p $MKELFPY &>/dev/null || MKELFPY=../vendor/semc/build/sin/mkelf.py
SEMCSC=semcsc.py
type -p $SEMCSC &>/dev/null || SEMCSC=../vendor/semc/build/sin/semcsc.py
CSH=create_sin_header
type -p $CSH &>/dev/null || CSH=../vendor/semc/build/sin/create_sin_header

FSCONFIG=$1
RAMDISK=$2
KERNEL=arch/arm/boot/zImage

# kernel.elf
$MKELFPY -o kernel-unsigned.elf $KERNEL@0x208000 $RAMDISK@0x1000000,ramdisk
$SEMCSC -c $FSCONFIG -p Kernel -t internal -i kernel-unsigned.elf -o kernel.elf

# kernel.si_
$CSH Kernel $FSCONFIG kernel.si_
cat < kernel.elf >> kernel.si_

# kernel.sin
$SEMCSC -c $FSCONFIG -p Kernel -t external -i kernel.si_ -o kernel.sin
