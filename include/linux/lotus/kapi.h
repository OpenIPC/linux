
#ifndef _LOTUS_KAPI_H_
#define _LOTUS_KAPI_H_

/*
 * NOTICE: Pls read this notice before adding a new API.
 * The APIs defined in this file may be used by external modules that
 * may be released in binary mode, so DO NOT depend on any structure and CONFIG_xxx
 * in kernel, otherwise, the external modules may not be able to load or causing
 * some kernel panic issue.
 */
char *lotus_get_cmd_line(void);

unsigned int get_chipid(void);
unsigned int get_cpu_freq(void);

int get_chip_temperature(void);
void lotus_set_cpu_volt_comp(unsigned int volt);
unsigned int lotus_get_cpu_volt(void);

#endif /* _LOTUS_KAPI_H_ */
