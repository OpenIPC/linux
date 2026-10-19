ifneq ($(CONFIG_PHYS_OFFSET),)
   zreladdr-y	:= $(shell printf "0x%08x" $$(($(CONFIG_PHYS_OFFSET) + 0x8000)))
else
   zreladdr-y	:= 0x80008000
endif
params_phys-y	:= 0x80000100

# The DTB the uImage carries (arch/arm/boot/Makefile, DTB_OBJS).
dtb-openipc-y	:= $(patsubst "%",%,$(CONFIG_AK_OPENIPC_DTB)).dtb
