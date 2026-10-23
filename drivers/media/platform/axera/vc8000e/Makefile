# SPDX-License-Identifier: GPL-2.0-only
#
# Build feature flags (see PROVENANCE.md for why each is on/off):
#   HANTROVCMD_ENABLE_IP_SUPPORT : on  -> ASIC_VCMD_SWREG_AMOUNT = 64, matches
#                                         the AX630C VCMD engine (hw_version_id
#                                         0x43421500)
#   SUPPORT_DMA_HEAP             : off -> reserved-memory dma pools instead
#   HANTROAXIFE_SUPPORT          : off -> no AXI-FE / MMU on this path
#   SUPPORT_WATCHDOG             : off -> keep the port minimal
ccflags-y += -DHANTROVCMD_ENABLE_IP_SUPPORT
ccflags-y += -I$(src)/eswin
# The vendored eswin source carries its own unused-symbol noise; it is
# deliberately kept unmodified so it can be re-synced from upstream eswin.
ccflags-y += -Wno-unused-variable -Wno-unused-function

obj-$(CONFIG_VIDEO_AX630C_VC8000E) += ax630c_venc_vcmd.o
ax630c_venc_vcmd-y := ax630c_vcmd_glue.o \
		      framebuf_alloc.o \
		      eswin/vc8000_vcmd_driver.o \
		      eswin/vcmdswhwregisters.o \
		      eswin/bidirect_list.o
