dtb-$(CONFIG_ARCH_THUNDER) += thunder-88xx.dtb
dtb-$(CONFIG_ARCH_VEXPRESS) += rtsm_ve-aemv8a.dtb foundation-v8.dtb esl_v8.dtb
dtb-$(CONFIG_ARCH_XGENE) += apm-mustang.dtb

targets += dtbs
targets += $(dtb-y)

BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB)).dtb.o
ifneq ($(CONFIG_BUILTIN_DTB),"")
obj-$(CONFIG_OF) += $(BUILTIN_DTB)
endif

dtbs: $(addprefix $(obj)/, $(dtb-y))

clean-files := *.dtb
