
#ifndef FH_STEPMOTOR_H_
#define FH_STEPMOTOR_H_

#include <linux/ioctl.h>

#include "fh_stepmotor_hw.h"


#define FH_SM_DEVICE_NAME                 "fh_stepmotor"
#define FH_SM_PLAT_DEVICE_NAME	"fh_stepmotor"

#define FH_SM_MISC_DEVICE_NAME                 "fh_stepmotor%d"



#define MAX_FHSM_NR (2)

#define FH_SM_IOCTL_MAGIC             's'
#define RESERVERD                   _IO(FH_SM_IOCTL_MAGIC, 0)
#define FH_SM_SET_PARAM             _IOWR(FH_SM_IOCTL_MAGIC, 1, __u32)
#define FH_SM_START_SYNC             _IOWR(FH_SM_IOCTL_MAGIC, 2, __u32)
#define FH_SM_STOP             _IOWR(FH_SM_IOCTL_MAGIC, 3, __u32)
#define FH_SM_SET_LUT             _IOWR(FH_SM_IOCTL_MAGIC, 4, __u32)
#define FH_SM_GET_LUT             _IOWR(FH_SM_IOCTL_MAGIC, 5, __u32)
#define FH_SM_GET_PARAM             _IOWR(FH_SM_IOCTL_MAGIC, 6, __u32)
#define FH_SM_START_ASYNC             _IOWR(FH_SM_IOCTL_MAGIC, 7, __u32)
#define FH_SM_GET_CUR_CYCLE             _IOWR(FH_SM_IOCTL_MAGIC, 8, __u32)





#endif /* FH_STEPMOTOR_H_ */
