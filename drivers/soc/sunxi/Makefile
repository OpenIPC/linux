obj-$(CONFIG_SUNXI_SRAM) +=	sunxi_sram.o
obj-y +=        sunxi-sid.o
ifeq ($(strip $(CONFIG_ARCH_SUN8IW8)     \
	      $(CONFIG_ARCH_SUN8IW19)    \
	      $(CONFIG_ARCH_SUN8IW21)    \
	      $(CONFIG_ARCH_SUN8IW16)    \
	      $(CONFIG_ARCH_SUN50IW10)   \
	      $(CONFIG_ARCH_SUN50IW11)),)
obj-$(CONFIG_PM) += pm.o
endif
obj-$(CONFIG_SUNXI_ARISC_INF) += arisc/
obj-y += sunxi-dump.o
obj-$(CONFIG_SUNXI_CPUFREQ_TEST) += vf-test.o
obj-y += vf-table.o
obj-$(CONFIG_GPU_PM_DOMAINS) += gpu_domain.o
obj-$(CONFIG_SUNXI_PM_DOMAINS) += pm_domains.o
obj-$(CONFIG_SUNXI_POWER_DOMAIN_TEST) += sunxi_pd_test.o
