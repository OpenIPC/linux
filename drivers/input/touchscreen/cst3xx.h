#ifndef TOUCHSCREEN_CST3XX__H
#define TOUCHSCREEN_CST3XX__H


/* -- dirver configure -- */
#define CFG_MAX_TOUCH_POINTS	1
#define MAX_AREA	0xff

#define CST3XX_NAME 	"cst3xx"

/*register address*/
#define CST3XX_ADDRESS 	0x1A


/* The platform data for the Focaltech ft5x0x touchscreen driver */
struct cst3xx_platform_data {
	unsigned int x_min;
	unsigned int y_min;
	unsigned int x_max;
	unsigned int y_max;
	unsigned long irqflags;
	unsigned int int_gpio;
	unsigned int reset_gpio;
};



#endif

