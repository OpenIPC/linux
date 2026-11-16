#ifndef __INGENIC_REBOOT_H__
#define __INGENIC_REBOOT_H__

enum restart_handler_priority {
	WDT_RESET_PROR = 0,
	RTC_HIBERNATE_RESET_PROR,
};

#define REBOOT_CMD_RECOVERY "recovery"
#define REBOOT_CMD_SOFTBURN "softburn"

void __weak ingenic_recovery_sign(void)
{
	pr_info("ingenic default recovery sign\n");
	return;
}

void __weak ingenic_reboot_sign(void)
{
	pr_info("ingenic default reboot sign\n");
	return;
}

void __weak ingenic_softburn_sign(void)
{
	pr_info("ingenic default softburn sign\n");
	return;
}
#endif /*__INGENIC_REBOOT_H__*/
