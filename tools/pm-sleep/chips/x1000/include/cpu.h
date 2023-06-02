#ifndef __CPU_H__
#define __CPU_H__
static void set_resume_pc(unsigned int resume_pc)
{
	unsigned int cpu_no,opcr;
	unsigned int reim,ctrl;
	unsigned int addr;
	/* set SLBC and SLPC */
	cpm_outl(1,CPM_SLBC);
	cpm_outl((unsigned int)resume_pc,CPM_SLPC);
	/* Clear previous reset status */
	cpm_outl(0,CPM_RSR);
}
#endif	/* __CPU_H__ */
