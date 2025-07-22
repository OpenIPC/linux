
#ifndef __INCLUDE_AHBM_
#define __INCLUDE_AHBM_

#define DDR_MC		0x130100E4
#define DDR_RESULT_1	0x130100D4
#define DDR_RESULT_2	0x130100D8
#define DDR_RESULT_3	0x130100DC
#define DDR_RESULT_4	0x130100E0

#define AHBM_CIM_IOB		0x130f0000
#define AHBM_AHB0_IOB	 	0x130f0100
#define AHBM_GPU_IOB 		0x130f0200
#define AHBM_LCD_IOB 		0x130f0300
#define AHBM_XXX_IOB 		0x13230000
#define AHBM_AHB2_IOB		0x134c0000

#define ahbm_restart(M)			\
do{ 					\
	outl(0x1,AHBM_##M##_IOB);	\
	outl(0x0,AHBM_##M##_IOB);	\
}while(0)

#define ahbm_stop(M)						\
do{ 								\
	outl(inl(AHBM_##M##_IOB) | 0x2,AHBM_##M##_IOB);		\
}while(0)

#define ahbm_read(M,off) inl((AHBM_##M##_IOB)+(off))

#endif


