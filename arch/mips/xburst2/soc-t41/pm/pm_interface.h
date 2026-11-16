#ifndef INTERFACE_H
#define INTERFACE_H



#define FIRMWARE_NAME	"pm_firmware.bin"
#define FIRMWARE_ADDR	0xb2400000
#define FIRMWARE_SPACE	32768


typedef int (*FUNC)(int args);
struct pm_param
{
	FUNC valid;
	FUNC begin;
	FUNC prepare;
	FUNC enter;
	FUNC finish;
	FUNC end;
	FUNC poweroff;

	int uart_index;
	int mcu_uart_idx;
	int mcu_uart_baud;
	suspend_state_t state;

	unsigned int resume_pc;
	unsigned int reload_pc;

	unsigned int load_addr;
	unsigned int load_space;

};
typedef void (*PMTEXTENTRY)(void* entry,struct pm_param **p);

#endif /* INTERFACE_H */
