obj-$(CONFIG_CRYPTO_DEV_SUN4I_SS) += sun4i-ss.o
sun4i-ss-y += sun4i/sun4i-ss-core.o sun4i/sun4i-ss-hash.o sun4i/sun4i-ss-cipher.o

obj-$(CONFIG_CRYPTO_DEV_SUNXI) += ss.o
obj-$(CONFIG_CRYPTO_DEV_SUNXI_IOCTL) += sunxi-ce-ioctl.o

sunxi-ce-ioctl-$(CONFIG_CRYPTO_DEV_SUNXI_IOCTL) += sunxi_ce_cdev.o
ss-$(CONFIG_CRYPTO_DEV_SUNXI) += sunxi_ss.o sunxi_ss_proc_comm.o

#ifdef CONFIG_ARCH_SUN8IW6
#	SUNXI_SS_VER = v2
#endif
ifdef CONFIG_ARCH_SUN8IW11
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW12
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW15
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW17
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW7
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW18
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN50I
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW16
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW19
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN50IW8
	SUNXI_SS_VER = v4
endif
ifdef CONFIG_ARCH_SUN50IW10
	SUNXI_SS_VER = v4
endif
ifdef CONFIG_ARCH_SUN8IW21
	SUNXI_SS_VER = v5
endif

ss-$(CONFIG_CRYPTO_DEV_SUNXI) +=  $(SUNXI_SS_VER)/sunxi_ss_reg.o $(SUNXI_SS_VER)/sunxi_ss_proc.o
sunxi-ce-ioctl-$(CONFIG_CRYPTO_DEV_SUNXI_IOCTL) += $(SUNXI_SS_VER)/sunxi_ss_reg.o $(SUNXI_SS_VER)/sunxi_ce_cdev_comm.o

ccflags-y += -Idrivers/crypto/sunxi-ss/$(SUNXI_SS_VER)

#ccflags-y += -DDEBUG
