#ifndef __CPUFREQ_H__
#define __CPUFREQ_H__
#include <common.h>
#include <uart.h>
#include <cpm.h>

void scale_cpu_freq(int status, unsigned int *cpccr)
{
	unsigned int val;

	if(status == SCALE) {
		/*
		 * (1) set L2CDIV = 7 CDIV = 3
		 */
		val = cpm_inl(CPM_CPCCR);
		*cpccr = val;
		val &= ~(0xff);
		val |= 0x73;
		val |= (1 << 22);
		cpm_outl(val, CPM_CPCCR);
		while((cpm_inl(CPM_CPCSR) & 1))
			serial_putc('U');
	} else if (status == RESTORE){
		val = *cpccr;
		val |= (1 << 22);
		cpm_outl(val, CPM_CPCCR);
		while((cpm_inl(CPM_CPCSR) & 1))
			serial_putc('U');
	}
}

#endif	/* __CPUFREQ_H__ */
