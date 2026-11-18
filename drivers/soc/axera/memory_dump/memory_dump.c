#include <asm/cacheflush.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/elf.h>
#include <linux/of_fdt.h>
#include <linux/delay.h>
#include <linux/crash_core.h>
#include <linux/reboot.h>
#include <linux/rtc.h>
#include <linux/libfdt.h>

#define DUMPINFO_OFFSET      1024
#define ALIGN_UP_16(v)       ((v + 0xF) & (~0xF))

struct arm_v8_mmu_regs {
	u64 sctlr_el1;
	u64 ttbr0_el1;
	u64 ttbr1_el1;
	u64 tcr_el1;
	u64 mair_el1;
	u64 amair_el1;
	u64 contextidr_el1;
	u64 reg_null;
};

struct axera_dump_info {
	unsigned long info_paddr;       /*dump info phy address*/
	unsigned long info_size;        /*dump info size*/
	unsigned long info_vaddr;       /*dump info virtual address*/
	unsigned long kernel_paddr;     /*kernel start phy address*/
	unsigned long kernel_vaddr;     /*kernel start virtual address*/
	unsigned long kmem_size;        /*kernel mem size*/
	unsigned long dump_info_addr;   /*dump info start address*/
	unsigned long magic_addr;       /*sysdump magic addr*/
	unsigned long time_addr;        /*sysdump time addr*/
	unsigned long dump_ranges_addr; /*dump ranges start address*/
	unsigned long pt_note_addr;     /*pt note address*/
	unsigned long mmu_addr;         /*mmu regs save addr*/
	unsigned int  cpu_id;           /*panic cpu id*/
	unsigned int  dump_cnt;         /*sysdump range count*/
};

static u32 s_os_mem_size;
static unsigned long s_note_sz;
static struct arm_v8_mmu_regs *s_axera_mmu_regs_ptr;
static struct axera_dump_info s_dump_info;

static int __init mem_setup(char *str)
{
	get_option(&str, &s_os_mem_size);
	printk("mem_setup = %s s_os_mem_size = %d\n", str, s_os_mem_size);
	return 1;
}

__setup("mem=", mem_setup);

static void axera_save_mmu_regs(struct arm_v8_mmu_regs *mmu_regs)
{
#ifdef CONFIG_ARM64
	u64 tmp = 0;
	asm volatile ("mrs %1, sctlr_el1\n\t"
		      "str %1, [%0]\n\t"
		      "mrs %1, ttbr0_el1\n\t"
		      "str %1, [%0, #8]\n\t"
		      "mrs %1, ttbr1_el1\n\t"
		      "str %1, [%0, #0x10]\n\t"
		      "mrs %1, tcr_el1\n\t"
		      "str %1, [%0, #0x18]\n\t"
		      "mrs %1, mair_el1\n\t"
		      "str %1, [%0, #0x20]\n\t"
		      "mrs %1, amair_el1\n\t"
		      "str %1, [%0, #0x28]\n\t"
		      "mrs %1, contextidr_el1\n\t" "str %1, [%0, #0x30]\n\t"::"r" (mmu_regs),
		      "r"(tmp):"%0", "%1");
#else
	asm volatile ("mrc    p15, 0, r1, c1, c0, 0\n\t"  /* SCTLR */
		"str r1, [%0]\n\t" "mrc    p15, 0, r1, c2, c0, 0\n\t"       /* TTBR0 */
		"str r1, [%0,#4]\n\t" "mrc    p15, 0, r1, c2, c0,1\n\t"     /* TTBR1 */
		"str r1, [%0,#8]\n\t" "mrc    p15, 0, r1, c2, c0,2\n\t"     /* TTBCR */
		"str r1, [%0,#12]\n\t" "mrc    p15, 0, r1, c3, c0,0\n\t"    /* DACR */
		"str r1, [%0,#16]\n\t" "mrc    p15, 0, r1, c5, c0,0\n\t"    /* DFSR */
		"str r1, [%0,#20]\n\t" "mrc    p15, 0, r1, c6, c0,0\n\t"    /* DFAR */
		"str r1, [%0,#24]\n\t" "mrc    p15, 0, r1, c5, c0,1\n\t"    /* IFSR */
		"str r1, [%0,#28]\n\t" "mrc    p15, 0, r1, c6, c0,2\n\t"    /* IFAR */
		"str r1, [%0,#32]\n\t":
		:           "r"(mmu_regs)
		:           "%r1", "memory");
#endif
}

void *axera_append_elf_note(Elf_Word *buf, char *name, unsigned int type, void *data, size_t data_len)
{
	struct elf_note *note = (struct elf_note *)buf;

	note->n_namesz = strlen(name) + 1;
	note->n_descsz = data_len;
	note->n_type   = type;
	buf += DIV_ROUND_UP(sizeof(*note), sizeof(Elf_Word));
	memcpy(buf, name, note->n_namesz);
	buf += DIV_ROUND_UP(note->n_namesz, sizeof(Elf_Word));
	memcpy(buf, data, data_len);
	buf += DIV_ROUND_UP(data_len, sizeof(Elf_Word));

	return (void*)buf;
}

void axera_crash_save_cpu(struct pt_regs *regs, int cpu)
{
	struct elf_prstatus prstatus;
	char core_name[8]={0};
	u32 *buf = (u32*)(s_dump_info.pt_note_addr + cpu*s_note_sz);

	if ((buf == NULL) || (cpu < 0) || (cpu >= nr_cpu_ids))
		return;

	snprintf(core_name, 32, "CORE%02d", cpu);
	memset(&prstatus, 0, sizeof(prstatus));
	if(regs != NULL)
		elf_core_copy_kernel_regs(&prstatus.pr_reg, regs);
	axera_append_elf_note(buf, core_name, NT_PRSTATUS, &prstatus, sizeof(prstatus));
}

void* axera_prepare_elf_headers(struct pt_regs *regs)
{
	unsigned int cpu;
	Elf_Phdr *phdr;
	Elf_Ehdr *ehdr;
	unsigned long notes_addr;
	unsigned long cur_notes_vaddr;

	ehdr = (Elf_Ehdr *)s_dump_info.info_vaddr;
	memcpy(ehdr->e_ident, ELFMAG, SELFMAG);
	ehdr->e_ident[EI_CLASS] = ELF_CLASS,
	ehdr->e_ident[EI_DATA] = ELF_DATA,
	ehdr->e_ident[EI_VERSION] = EV_CURRENT,
	ehdr->e_ident[EI_OSABI] = ELF_OSABI,
	ehdr->e_type = ET_CORE;
	ehdr->e_machine = ELF_ARCH;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_phoff = sizeof(Elf_Ehdr);
	ehdr->e_ehsize = sizeof(Elf_Ehdr);
	ehdr->e_phentsize = sizeof(Elf_Phdr);
	/* Prepare one phdr of type PT_NOTE for each present cpu */
	phdr = (Elf_Phdr*)(ehdr + 1);
	notes_addr = s_dump_info.pt_note_addr - s_dump_info.info_vaddr;
	cur_notes_vaddr = s_dump_info.pt_note_addr;
	phdr->p_filesz = 0;
	for_each_present_cpu(cpu) {
		phdr->p_filesz += s_note_sz;
		cur_notes_vaddr += s_note_sz;
	}
	/* Prepare one PT_NOTE header for vmcoreinfo */
	phdr->p_type = PT_NOTE;
	phdr->p_offset = phdr->p_paddr = notes_addr;
	phdr->p_filesz += vmcoreinfo_size;
	phdr->p_memsz = phdr->p_filesz;
	phdr->p_vaddr = s_dump_info.kernel_vaddr;
	phdr->p_paddr = s_dump_info.kernel_paddr;
	memcpy((void*)cur_notes_vaddr, vmcoreinfo_note, vmcoreinfo_size);
	(ehdr->e_phnum)++;
	/* kernel dump */
	phdr ++;
	phdr->p_type = PT_LOAD;
	phdr->p_flags = PF_R | PF_W | PF_X;
	phdr->p_offset = s_dump_info.info_size;
	phdr->p_vaddr = s_dump_info.kernel_vaddr;
	phdr->p_paddr = s_dump_info.kernel_paddr;
	phdr->p_filesz = s_dump_info.kmem_size;
	phdr->p_memsz = s_dump_info.kmem_size;
	phdr->p_align = 0;
	(ehdr->e_phnum)++;

	return (void*)++phdr;
}

static inline void axera_crash_setup_regs(struct pt_regs *newregs,
                                    struct pt_regs *oldregs)
{
        if (oldregs) {
                memcpy(newregs, oldregs, sizeof(*newregs));
        } else {
#ifdef CONFIG_ARM64
                u64 tmp1, tmp2;

                __asm__ __volatile__ (
                        "stp     x0,   x1, [%2, #16 *  0]\n"
                        "stp     x2,   x3, [%2, #16 *  1]\n"
                        "stp     x4,   x5, [%2, #16 *  2]\n"
                        "stp     x6,   x7, [%2, #16 *  3]\n"
                        "stp     x8,   x9, [%2, #16 *  4]\n"
                        "stp    x10,  x11, [%2, #16 *  5]\n"
                        "stp    x12,  x13, [%2, #16 *  6]\n"
                        "stp    x14,  x15, [%2, #16 *  7]\n"
                        "stp    x16,  x17, [%2, #16 *  8]\n"
                        "stp    x18,  x19, [%2, #16 *  9]\n"
                        "stp    x20,  x21, [%2, #16 * 10]\n"
                        "stp    x22,  x23, [%2, #16 * 11]\n"
                        "stp    x24,  x25, [%2, #16 * 12]\n"
                        "stp    x26,  x27, [%2, #16 * 13]\n"
                        "stp    x28,  x29, [%2, #16 * 14]\n"
                        "mov     %0,  sp\n"
                        "stp    x30,  %0,  [%2, #16 * 15]\n"

                        "/* faked current PSTATE */\n"
                        "mrs     %0, CurrentEL\n"
                        "mrs     %1, SPSEL\n"
                        "orr     %0, %0, %1\n"
                        "mrs     %1, DAIF\n"
                        "orr     %0, %0, %1\n"
                        "mrs     %1, NZCV\n"
                        "orr     %0, %0, %1\n"
                        /* pc */
                        "adr     %1, 1f\n"
                "1:\n"
                        "stp     %1, %0,   [%2, #16 * 16]\n"
                        : "=&r" (tmp1), "=&r" (tmp2)
                        : "r" (newregs)
                        : "memory"
                );
#else
		__asm__ __volatile__("stmia  %[regs_base], {r0-r12}\n\t"
			"mov    %[_ARM_sp], sp\n\t"
			"str    lr, %[_ARM_lr]\n\t"
			"adr    %[_ARM_pc], 1f\n\t"
			"mrs    %[_ARM_cpsr], cpsr\n\t"
			"1:":[_ARM_pc] "=r"(newregs->ARM_pc),
			     [_ARM_cpsr] "=r"(newregs->ARM_cpsr),
			     [_ARM_sp] "=r"(newregs->ARM_sp),[_ARM_lr] "=o"(newregs->ARM_lr)
			    :[regs_base] "r"(&newregs->ARM_r0)
			    :"memory");
#endif
        }
}

void axera_save_memory_dump(void)
{
	unsigned int cpu;
	struct timespec64 txc;
	struct rtc_time tm;
	u32 reason_mask = 0xf98e7c6d;
	struct pt_regs newregs;
	void *meminfo_ptr;
	unsigned long addr;

	bust_spinlocks(1);
	axera_crash_setup_regs(&newregs, NULL);
#ifdef CONFIG_CRASH_CORE
	crash_save_vmcoreinfo();
#endif
#ifdef CONFIG_KEXEC
	machine_crash_shutdown(&newregs);
#endif
	meminfo_ptr = axera_prepare_elf_headers(&newregs);
	s_dump_info.cpu_id = smp_processor_id();
	addr = (unsigned long)meminfo_ptr + sizeof(struct axera_dump_info);
	if (s_dump_info.dump_ranges_addr < ALIGN_UP_16(addr)){
		pr_warn("[warn] sysdump address overlay!!!");
	}
	memcpy((void*)s_dump_info.dump_info_addr, &s_dump_info, sizeof(s_dump_info));
	ktime_get_real_ts64(&txc);
	txc.tv_sec -= sys_tz.tz_minuteswest * 60;
	rtc_time64_to_tm(txc.tv_sec, &tm);
	tm.tm_mon += 1;
	pr_err("kernel panic time : %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900,
	       tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	memcpy((void *)(s_dump_info.time_addr), &tm, sizeof(tm));
	memcpy((void *)(s_dump_info.magic_addr), &reason_mask, sizeof(reason_mask));
	s_axera_mmu_regs_ptr = (struct arm_v8_mmu_regs *)(s_dump_info.mmu_addr);
	axera_save_mmu_regs(s_axera_mmu_regs_ptr);
	for_each_present_cpu(cpu) {
		axera_crash_save_cpu(NULL, cpu);
	}
	axera_crash_save_cpu(&newregs,smp_processor_id());
	mdelay(100);
#ifdef CONFIG_ARM64
	crash_smp_send_stop();
#else
	system_state = SYSTEM_RESTART;
	smp_send_stop();
#endif
	mdelay(2000);
#ifdef CONFIG_ARM64
	axera_armv8_flush_cache_all();
#else
	flush_cache_all();
#endif
	mdelay(1000);
	bust_spinlocks(0);
}

static int __init axera_memory_dump_init(void)
{
	int offset;
	int len, i;
	int cnt;
	const u32 *val;
	unsigned long addr, size, memory_addr;
	unsigned long dump_info_vaddr;
	u32 *tmp;

	void *fdt = initial_boot_params;

	struct device_node *of_node = of_find_compatible_node(NULL, NULL, "axera_memory_dump");
	if (!of_node) {
		pr_err("%s:can't find memory_dump tree node\n", __func__);
		return -ENODEV;
	}

	if (fdt_check_header(fdt)) {
		pr_err("Invalid device tree header \n");
		return -ENODEV;
	}
	offset = fdt_path_offset(fdt, "/reserved-memory/axera_memory_dump@0");
	if (offset < 0) {
		pr_err("dtb get reserved_mem info error \n");
		return -ENODEV;
	}

	val = fdt_getprop(fdt, offset, "reg", &len);

#ifdef CONFIG_ARM64
	addr = fdt32_to_cpu(val[0]);
	addr = addr << 32;
	addr |= fdt32_to_cpu(val[1]);
	size = fdt32_to_cpu(val[2]);
	size = size << 32;
	size |= fdt32_to_cpu(val[3]);
#else
	addr = fdt32_to_cpu(val[1]);
	size = fdt32_to_cpu(val[3]);
#endif
	dump_info_vaddr = (unsigned long)ioremap_wc(addr,size);
	if (dump_info_vaddr == 0x0)
		return -EIO;
	memset((void *)dump_info_vaddr, 0, size);
	s_dump_info.info_paddr     = addr;
	s_dump_info.info_size      = size;
	s_dump_info.info_vaddr     = (unsigned long)dump_info_vaddr;
	s_dump_info.dump_info_addr = ALIGN_UP_16(s_dump_info.info_vaddr + DUMPINFO_OFFSET);
	s_dump_info.magic_addr     = ALIGN_UP_16(s_dump_info.dump_info_addr + sizeof(struct axera_dump_info));
	s_dump_info.time_addr      = ALIGN_UP_16(s_dump_info.magic_addr + sizeof(int));
	/*dump ranges*/
	val = fdt_getprop(fdt, offset, "dump_ranges", &len);
	if (val == NULL) {
		pr_err("get dump_ranges failed! len=%d\n", len);
		return -ENODEV;
	}
	s_dump_info.dump_ranges_addr = ALIGN_UP_16(s_dump_info.time_addr + sizeof(struct rtc_time));
	cnt = len / (4 * sizeof(int));
	s_dump_info.dump_cnt = cnt;
	tmp = (u32*)s_dump_info.dump_ranges_addr;
	for (i=0; i < cnt; i++){
		*tmp++ = fdt32_to_cpu(val[i*4]);
		*tmp++ = fdt32_to_cpu(val[i*4 + 1]);
		*tmp++ = fdt32_to_cpu(val[i*4 + 2]);
		*tmp++ = fdt32_to_cpu(val[i*4 + 3]);
	}
	s_dump_info.pt_note_addr = ALIGN_UP_16(s_dump_info.dump_ranges_addr + cnt*4*sizeof(int));
	s_dump_info.mmu_addr     = ALIGN_UP_16(s_dump_info.pt_note_addr + sizeof(note_buf_t)*NR_CPUS + VMCOREINFO_NOTE_SIZE);

	offset = fdt_path_offset(fdt, "/memory@40000000");
	if (offset < 0)
		pr_err("memory node error \n");
	val = fdt_getprop(fdt, offset, "reg", &len);

#ifdef CONFIG_ARM64
	memory_addr = fdt32_to_cpu(val[0]);
	memory_addr = memory_addr << 32;
	memory_addr |= fdt32_to_cpu(val[1]);
#else
	memory_addr = fdt32_to_cpu(val[1]);
#endif
	s_dump_info.kernel_paddr = memory_addr;
	s_dump_info.kmem_size    = s_os_mem_size * SZ_1M;
	s_dump_info.kernel_vaddr = (unsigned long)phys_to_virt(memory_addr);
	s_note_sz = round_up(sizeof(struct elf_note), sizeof(Elf_Word));
	s_note_sz += round_up(strlen("CORE00"), sizeof(Elf_Word));
	s_note_sz += round_up(sizeof(struct elf_prstatus), sizeof(Elf_Word));

	BUG_ON((s_dump_info.kernel_paddr + s_dump_info.kmem_size) < (s_dump_info.info_paddr + s_dump_info.info_size));

	return 0;
}

module_init(axera_memory_dump_init);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xuxiyang@axera-tech.com");
MODULE_VERSION("1.0");
