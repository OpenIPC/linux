#!/bin/bash
cp $1 arch/mips/boot/vmlinux.bin.bk
./scripts/ingenic_hardlzma/lzma -z -k -f -9 arch/mips/boot/vmlinux.bin.bk
cp jz_lzma_out.bin $2
rm jz_lzma_out.bin arch/mips/boot/vmlinux.bin.bk
sync

