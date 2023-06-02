#ifndef FH_PREDEFINED_H_
#define FH_PREDEFINED_H_

typedef unsigned char		UINT8;
typedef unsigned short	UINT16;
typedef unsigned int		UINT32;
typedef unsigned long long	UINT64;

typedef char			SINT8;
typedef short			SINT16;
typedef int			SINT32;
typedef long long		SINT64;
#define FALSE 			(0)
#define TRUE 			(!FALSE)
#define reg_read(addr) (*((volatile UINT32 *)(addr)))
#define reg_write(addr,value) (*(volatile UINT32 *)(addr)=(value))

#define GET_REG(addr) reg_read(addr)
#define SET_REG(addr,value) reg_write(addr,value)
#define SET_REG_M(addr,value,mask) reg_write(addr,(reg_read(addr)&(~(mask)))|((value)&(mask)))
#define SET_REG_B(addr,element,highbit,lowbit) SET_REG_M((addr),((element)<<(lowbit)),(((1<<((highbit)-(lowbit)+1))-1)<<(lowbit)))

#define GET_REG8(addr) (*((volatile UINT8 *)(addr)))
#define SET_REG8(addr,value) (*(volatile UINT8 *)(addr)=(value))

#define LD8(addr) 		(*((volatile u8 *)(addr)))
#define ST8(addr,value)		(*(volatile u8 *)(addr)=(value))
#define LD16(addr) 		(*((volatile u16 *)(addr)))
#define ST16(addr,value)	(*(volatile u16 *)(addr)=(value))
#define LD32(addr)		(*((volatile u32 *)(addr)))
#define ST32(addr,value)	(*(volatile u32 *)(addr)=(value))

#endif /* FH_PREDEFINED_H_ */
