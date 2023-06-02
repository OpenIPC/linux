
#ifndef FH_CLK_MISCDEV_H_
#define FH_CLK_MISCDEV_H_

#include <mach/clock.h>

#define DEVICE_NAME		"fh_clk_miscdev"

#define CLK_IOCTL_MAGIC 		'c'
#define ENABLE_CLK	 		_IOWR(CLK_IOCTL_MAGIC, 0, unsigned int)
#define DISABLE_CLK		 	_IOWR(CLK_IOCTL_MAGIC, 1, unsigned int)
#define SET_CLK_RATE			_IOWR(CLK_IOCTL_MAGIC, 2, unsigned int)
#define GET_CLK_RATE			_IOWR(CLK_IOCTL_MAGIC, 3, unsigned int)
#define SET_PMU				_IOWR(CLK_IOCTL_MAGIC, 4, unsigned int)
#define GET_PMU				_IOWR(CLK_IOCTL_MAGIC, 5, unsigned int)

#define CLK_IOCTL_MAXNR 		8


#endif /* FH_CLK_MISCDEV_H_ */
