#
# Makefile for the linux ramfs routines.
#

obj-y += ramfs.o

ifneq ($(CONFIG_MMU),y)
file-mmu-y := file-nommu.o
endif
file-mmu-$(CONFIG_MMU) := file-mmu.o
ramfs-objs += inode.o $(file-mmu-y)
