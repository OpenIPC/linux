#ifndef IOMUX_H_
#define IOMUX_H_
#include <linux/types.h>
#include <linux/io.h>
#include <mach/io.h>

#define IOMUX_PADTYPE(n)		(Iomux_PadType##n *)
#define IOMUX_PUPD_NONE			0
#define IOMUX_PUPD_DOWN			1
#define IOMUX_PUPD_UP			2
#define IOMUX_PUPD_KEEPER		3
//#define IOMUX_DEBUG


typedef union {
	struct {
		__u32	sr		: 1;
		__u32	reserved_3_1	: 3;

		__u32	e8_e4		: 2;
		__u32	reserved_31_6	: 24;

	} bit;
	__u32 dw;
} Iomux_PadType5;

typedef union {
	struct {
		__u32	sr		: 1;
		__u32	reserved_3_1	: 3;

		__u32	e8_e4		: 2;
		__u32	reserved_7_6	: 2;

		__u32	mfs		: 1;
		__u32	reserved_31_9	: 23;

	} bit;
	__u32 dw;
} Iomux_PadType8;


typedef union {
	struct {
		__u32	smt		: 1;
		__u32	reserved_3_1	: 3;

		__u32	ie		: 1;
		__u32	reserved_7_5	: 3;

		__u32	pu_pd		: 2;
		__u32	reserved_31_10	: 22;

	} bit;
	__u32 dw;
} Iomux_PadType9;


typedef union {
	struct {
		__u32	e4_e2		: 2;
		__u32	reserved_3_2	: 2;

		__u32	smt		: 1;
		__u32	reserved_7_5	: 3;

		__u32	ie		: 1;
		__u32	reserved_11_9	: 3;

		__u32	mfs		: 2;
		__u32	reserved_31_14	: 18;

	} bit;
	__u32 dw;
} Iomux_PadType13;

typedef union {
	struct {
		__u32	sr		: 1;
		__u32	reserved_3_1	: 3;

		__u32	e8_e4		: 2;
		__u32	reserved_7_6	: 2;

		__u32	smt		: 1;
		__u32	reserved_11_9	: 3;

		__u32	ie		: 1;
		__u32	e		: 1;	//only for PAD_MAC_REF_CLK_CFG (0x00a4)
		__u32	reserved_15_12	: 2;

		__u32	pu_pd		: 2;
		__u32	reserved_31_18	: 14;

	} bit;
	__u32 dw;
} Iomux_PadType17;

typedef union {
	struct {
		__u32	sr		: 1;
		__u32	reserved_3_1	: 3;

		__u32	e4_e2		: 2;
		__u32	reserved_7_6	: 2;

		__u32	smt		: 1;
		__u32	reserved_11_9	: 3;

		__u32	ie		: 1;
		__u32	reserved_15_13	: 3;

		__u32	pu_pd		: 2;
		__u32	reserved_19_18	: 2;

		__u32	mfs		: 1;
		__u32	reserved_31_21	: 11;

	} bit;
	__u32 dw;
} Iomux_PadType20;


typedef union {
	struct {
		__u32	sr		: 1;
		__u32	reserved_3_1	: 3;

		__u32	e4_e2		: 2;
		__u32	reserved_7_6	: 2;

		__u32	smt		: 1;
		__u32	reserved_11_9	: 3;

		__u32	ie		: 1;
		__u32	reserved_15_13	: 3;

		__u32	pu_pd		: 2;
		__u32	reserved_19_18	: 2;

		__u32	mfs		: 2;
		__u32	reserved_31_21	: 10;

	} bit;
	__u32 dw;
} Iomux_PadType21;

typedef struct {
	u32 *reg;
	u32 reg_offset;
	char *func_name[4];
	int reg_type;
	int func_sel;
	int drv_cur;
	int pupd;
} Iomux_Pad;

typedef struct {
	void __iomem *base;
	Iomux_Pad *pads;
} Iomux_Object;

void fh_iomux_init(Iomux_Object *iomux_obj);

#endif /* IOMUX_H_ */
