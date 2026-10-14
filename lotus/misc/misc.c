
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>

void dump_reg(char *head, u8 *v_addr, phys_addr_t p_addr, u32 length, char *tail)
{
	u32 i;

	if (!v_addr) {
		pr_info("Invalid v addr\n");
		return;
	}

	if (head)
		printk("%s\n", head);

	for (i = 0; i < length; i += 4) {
		printk("0x%X: %08x\n", p_addr + i, *((u32 *)(v_addr + i)));
	}

	if (tail)
		printk("%s\n", tail);
}

