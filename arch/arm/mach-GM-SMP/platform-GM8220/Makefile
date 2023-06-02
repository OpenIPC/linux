#
# Makefile for the GM platform dependent files

# Object file lists.

obj-y		:= board.o pmu.o platform.o

ifeq ($(CONFIG_FTTMR010),y)
obj-y		+= timer_fixup.o
endif
