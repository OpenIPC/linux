/**
 * @file bridge.h
 * @brief type definition of KDRV API.
 */

#ifndef __BRIDGE_H__
#define __BRIDGE_H__

extern int kdrv_bridge_map(void);
extern int kdrv_bridge_unmap(void);
extern int kdrv_bridge_get_tag(unsigned int tag, unsigned int *p_val);

#endif
