ifneq ($(CONFIG_PHYS_OFFSET),)
   zreladdr-y	:= $(shell printf "0x%08x" $$(($(CONFIG_PHYS_OFFSET) + 0x8000)))
else
   zreladdr-y	:= 0x80008000
endif
params_phys-y	:= 0x80000100

# The DTB the uImage carries (arch/arm/boot/Makefile, DTB_OBJS).
dtb-openipc-$(CONFIG_MACH_AK3918EV200)	:= ak3918ev200-demb.dtb
