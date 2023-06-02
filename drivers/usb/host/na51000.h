#ifndef NVTUSBNA51000_REGS
#define NVTUSBNA51000_REGS

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/writeback.h>


void xhci_nvt_host_init(struct platform_device *pdev);
void xhci_nvt_host_uninit(struct platform_device *pdev);

#endif

