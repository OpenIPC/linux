#ifndef __COMMON_H__
#define __COMMON_H__
void udelay(int d);
#define REG32(x) (*(volatile unsigned int *)(x))
#endif /* __COMMON_H__ */
