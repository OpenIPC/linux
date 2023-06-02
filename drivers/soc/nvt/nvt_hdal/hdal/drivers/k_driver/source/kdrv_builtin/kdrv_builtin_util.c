#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>

static int m_rtos_boot = 0;
static int m_fastboot = 0;
static int m_fastboot_init = 0;

#if !defined(MODULE)
static int __init early_param_rtos_boot(char *p)
{
	if (p == NULL) {
		return 0;
	}
	if (strncmp(p, "on", 3) == 0) {
		m_rtos_boot = 1;
	}
	return 0;
}

early_param("rtos_boot", early_param_rtos_boot);
#endif

int kdrv_builtin_is_fastboot(void)
{
	if (m_fastboot_init) {
		return m_fastboot;
	}

	if (m_rtos_boot) {
		struct device_node* of_node = of_find_node_by_path("/fastboot");

		if (of_node) {
			if (of_property_read_u32(of_node, "enable", &m_fastboot) != 0) {
				pr_err("cannot find /fastboot/enable");
			}
		}
	}

	m_fastboot_init = 1;

	return m_fastboot;
}

EXPORT_SYMBOL(kdrv_builtin_is_fastboot);

#else
#endif
