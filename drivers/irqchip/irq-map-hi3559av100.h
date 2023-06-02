#ifndef __IRQ_MAP_CONFIG_H__
#define __IRQ_MAP_CONFIG_H__

#define TO_A53MP0  (1<<0x00)
#define TO_A53MP1  (1<<0x01)
#define TO_A73MP0  (1<<0x02)
#define TO_A73MP1  (1<<0x03)
#define TO_A53UP_  (1<<0x04)  //Local
const unsigned char irq_map[1024] = {
    0,         0,         0,         0,
    0,         0,         0,         0,
    0,         0,         0,         0,
    0,         0,         0,         0,
    0,         0,         0,         0,
    0,         0,         0,         0,
    0,         0,         0,         0,
    0,         0,         0,         0,
	/*time 0   timer 2    timer 4    timer 6  */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53UP_, /* 32  ~ 35 */
	/*time 8   timer 10   uart 0     uart 1   */
	TO_A53UP_, TO_A53UP_, TO_A53MP0, TO_A53UP_, /* 36  ~ 39 */
	/*uart 2   uart 3     uart 4     RTC      */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 40  ~ 43 */
	/*i2c 0    i2c 1      i2c 2      i2c 3    */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 44  ~ 47 */
	/*i2c 4    i2c 5      i2c 6      i2c 7    */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 48  ~ 51 */
	/*i2c 8    i2c 9      i2c 10     i2c 11   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 52  ~ 55 */
	/*IR       FMC        eMMC       VDMA   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 56  ~ 59 */
	/*res      res        SPACC      SSP0   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 60  ~ 63 */
	/*SSP1     SSP2       SSP3       SSP4   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 64  ~ 67 */
	/*GMAC0    GMAC1      SOFTWARE   VEDU0   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 68  ~ 71 */
	/*VEDU1    VEDU2      res        VGS0   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 72  ~ 75 */
	/*VGS1     VPSS0      VPSS1      GDC0   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 76  ~ 79 */
	/*GDC1     JPGE       res        GME   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 80  ~ 83 */
	/*JPGD     TDE        GZIP       PGD   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 84  ~ 87 */
	/*IVE      res        NNIE0      NNIE1   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 88  ~ 91 */
	/*VICAP    VIPROC0    VIPROC1    MIPI TX   */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 92  ~ 95 */
	/*HDMI     HDMI       HDMI       VDP0 */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0, /* 96  ~ 99 */
	/*VDP1     AIAO       DSP0       DSP1 */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 100 ~ 103*/
	/*DSP2     DSP3       SDIO0      SDIO1  */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 104 ~ 107*/
	/*SDIO2    SDIO_WA0   SDIO_WA1   SDIO_WA2  */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 108 ~ 111*/
	/*DDRPHY   DMAC0      DMAC1      UFS    */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 112 ~ 115*/
	/*USB 0    USB 1      SLVS-EC    res    */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 116 ~ 119*/
	/*MIPI RX  DDRT 0     DDRT 1     VDH_OLP    */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 120 ~ 123*/
	/*VDH_ILP  VDH_SAFE   VDH_NOR    VDH_MDMA    */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 124 ~ 127*/
	/*SSP5     SSP6       A53UP CCI  BL_CCI  */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 128 ~ 131*/
	/*GPU      GPU        GPU        GPU     */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 132 ~ */
	/*RSA      WDG        CAN 0      CAN 1   */
	TO_A53MP0, TO_A53MP0    \
               | TO_A53MP0  \
               | TO_A53MP1  \
               | TO_A73MP0  \
               | TO_A73MP1  \
                        , TO_A53MP0, TO_A53MP0,	/* 136 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 140 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 144 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 148 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 152 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 156 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 160 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 164 ~ */
	/*A73MP_NCOMMIRQ1 PMU CTIIRQ0    COMMRX 0*/
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 168 ~ */
	/* COMMTX  NCOMMIRQ   PCIE       PCIE    */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 172 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 176 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 180 ~ */
	/* AVSP 0  AVSP 1     AVSP 2     AVSP 3  */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 184 ~ */
	/* AVSP 4  AVSP 5     AVSP 6     AVSP 7  */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 188 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 192 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 196 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 200 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 204 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 208 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 212 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 216 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 220 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 224 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 228 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 232 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 236 ~ */
	/*DPU_RECT DPU_MATCH  DPU_POSTPROC IPC_0  */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 240 ~ */
	/*IPC_1    IPC_2*/
	TO_A53MP0, TO_A53UP_, TO_A53MP0, TO_A53MP0,	/* 244 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 248 ~ */
	TO_A53MP0, TO_A53MP0, TO_A53MP0, TO_A53MP0,	/* 252 ~ */
};/* [32 - 255] */
#else
#error "should not include irq-map-hi3559av100.h twice"
#endif
