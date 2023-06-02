/**
 * @file kdrv_builtin.h
 * @brief type definition of KDRV API.
 */

#ifndef __KDRV_BUILTIN_H__
#define __KDRV_BUILTIN_H__

/*!
 * @fn int kdrv_builtin_is_fastboot(void)
 * @brief indicate start linux with fastboot mode
 * @return return 0: start linux by uboot, 1: start linux by rtos with fastboot
 */
extern int kdrv_builtin_is_fastboot(void);


#endif
