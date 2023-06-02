#!/bin/sh
make -f scripts/sstar/.cscope.mk gen_db 2>&1 | grep -v warning
