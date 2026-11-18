# The SDK build passes PROJECT=<board>; otherwise build every board here.
ifneq ($(PROJECT),)
dtb-$(CONFIG_ARCH_AXERA) += $(PROJECT).dtb
else
dtb-$(CONFIG_ARCH_AXERA) += $(patsubst %.dts,%.dtb,$(notdir $(wildcard $(srctree)/$(src)/*.dts)))
endif
