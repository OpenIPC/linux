
#ifndef _LOTUS_MISC_H_
#define _LOTUS_MISC_H_

#ifdef _LOTUS_DUMP_REG
void dump_reg(char *head, u8 *v_addr, phys_addr_t p_addr, u32 length, char *tail);
#else
#define dump_reg(h, v, p, l, t)
#endif

#endif /* _LOTUS_MISC_H_ */
