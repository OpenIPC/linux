#!/bin/bash
#xxd -l 64 $1
./scripts/ingenic_hardlzma/ingenic_mkimage -i $1
#xxd -l 64 $1

