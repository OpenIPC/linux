   zreladdr-y	:= 0x80008000
params_phys-y	:= 0x80000100

# The DTB the uImage carries (arch/arm/boot/Makefile, DTB_OBJS).
dtb-openipc-y	:= $(addsuffix .dtb,$(patsubst "%",%,$(CONFIG_AK_OPENIPC_DTB)))
