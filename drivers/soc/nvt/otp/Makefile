obj-$(CONFIG_NVT_OTP_SUPPORT) += nvt_otp.o

ifeq ($(CONFIG_NVT_IVOT_PLAT_NA51068), y)
nvt_otp_protect := na51068
else ifeq ($(CONFIG_NVT_IVOT_PLAT_NA51089), y)
nvt_otp_protect := na51089
else
nvt_otp_protect := na51055
endif

ifdef CONFIG_FUNCTION_TRACER
ORIG_CFLAGS := $(KBUILD_CFLAGS)
KBUILD_CFLAGS = $(subst $(CC_FLAGS_FTRACE),,$(ORIG_CFLAGS))
endif



ifeq ($(CONFIG_OUTER_CACHE),)
nvt_otp-objs := $(nvt_otp_protect)/otp_protected.o_shipped otp_drv.o otp_main.o otp_platform.o otp_proc.o
else
nvt_otp-objs := $(nvt_otp_protect)/otp_protected.o_shipped otp_drv.o otp_main.o otp_platform.o otp_proc.o
endif
