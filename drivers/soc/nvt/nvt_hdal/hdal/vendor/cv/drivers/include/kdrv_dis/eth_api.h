#ifndef __eth_api_h_
#define __eth_api_h_

#include "dis_drv.h"

//#define DEBUG_KDRV_ETH


int exam_dis_eth_ipe_cb(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv);
int exam_dis_eth_ipe_cb_un(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv);

#ifdef DEBUG_KDRV_ETH
int vendor_dis_eth_ipe_cb(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv);
int vendor_dis_eth_ipe_cb_un(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv);
int vendor_dis_eth_get(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv);
#endif
#endif
