#ifndef RTO_DAVINCI
#define RTO_DAVINCI


#define DAVINCI_RTO_PHIS 0x01C20C00
typedef struct {
	unsigned long id;
	unsigned long ctrl_status;
} davinci_rto;


#define ENABLE_RTO (1<<0)
#define SELECTBIT_RTO12 (0<<1)
#define SELECTBIT_RTO34 (1<<1)
#define DETECTBIT_MASK_RTO (3<<5)
#define DETECTBIT_NE_RTO (0)
#define DETECTBIT_RE_RTO (1<<5)
#define DETECTBIT_FE_RTO (2<<5)
#define DETECTBIT_BE_RTO (3<<5)
#define OUTPUTEMODE_RTO_DIRECT (0<<7)
#define OUTPUTEMODE_RTO_TOGGLE (1<<7)
#define OPPATERNDATA_RTO0 (1<<8)
#define OPPATERNDATA_RTO1 (1<<9)
#define OPPATERNDATA_RTO2 (1<<10)
#define OPPATERNDATA_RTO3 (1<<11)
#define OPMASKDATA_RTO0 (1<<12)
#define OPMASKDATA_RTO1 (1<<13)
#define OPMASKDATA_RTO2 (1<<14)
#define OPMASKDATA_RTO3 (1<<15)
#define OVERRUN_RTO (1<<16)
#define SOURCEPOLARITY_RTO (1<<17)



#endif //RTO_DAVINCI
