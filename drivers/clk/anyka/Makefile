#
# Anyka Clock specific Makefile
#
obj-$(CONFIG_MACH_AK39EV330) += ak39ev330_clk.o
# AK3918EV200 reuses the AK39EV330 clock driver verbatim: the 3-PLL tree and
# the 15-bit ASIC clock-gate bitmap are identical on this silicon. The
# EV200 PLL divider delta is entirely in DT (&core_pll clock-div-od/
# clock-frequency), not in this file.
obj-$(CONFIG_MACH_AK3918EV200) += ak39ev330_clk.o
obj-$(CONFIG_MACH_AK3918AV130) += ak3918av100_fixed_clk.o ak3918av100_factor_clk.o \
				  ak3918av100_gate_clk.o ak3918av100_mux_clk.o
