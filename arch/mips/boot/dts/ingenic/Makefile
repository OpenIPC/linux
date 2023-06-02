dtb-$(CONFIG_JZ4740_QI_LB60)	+= qi_lb60.dtb
dtb-$(CONFIG_JZ4780_CI20)	+= ci20.dtb
dtb-$(CONFIG_DT_HALLEY2_V20)	+= halley2v20.dtb
dtb-$(CONFIG_DT_SEAL)	+= seal.dtb
dtb-$(CONFIG_DT_X2000_V12_FPGA)	+= x2000_v12_fpga.dtb

ifdef CONFIG_FAST_BOOT
dtb-$(CONFIG_DT_T40_SHARK)	+= shark_fast.dtb
else
dtb-$(CONFIG_DT_T40_SHARK)	+= shark.dtb
endif
obj-y				+= $(patsubst %.dtb, %.dtb.o, $(dtb-y))

# Force kbuild to make empty built-in.o if necessary
obj-				+= dummy.o

always				:= $(dtb-y)
clean-files			:= *.dtb *.dtb.S
