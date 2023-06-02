#
# Makefile for the GM platform dependent files

# Object file lists.

ifeq ($(CONFIG_GM8210_FPGA),y)
obj-y		:= board.o pmu_fpga.o timer_fixup.o platform.o
else
obj-y		:= board.o pmu.o timer_fixup.o platform.o
obj-$(CONFIG_GM8312)   += pmu_pcie.o 
endif
