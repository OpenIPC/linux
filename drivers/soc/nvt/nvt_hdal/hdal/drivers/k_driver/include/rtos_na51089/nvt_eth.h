#ifndef _NVTIMETH_DEF_H
#define _NVTIMETH_DEF_H

#include <kwrap/nvt_type.h>

/**
    ethernet link status

    @note For NVTIMETH_LINK_CB
*/
#define NVTETH_LINK_DOWN          (0x01)          //< link down
#define NVTETH_LINK_UP            (0x02)          //< link up

/**
    ethernet duplex mode

    @note For nvtimeth_set_duplex()
*/
#define NVTETH_DUPLEX_HALF        (0x01)          //< half duplex
#define NVTETH_DUPLEX_FULL        (0x02)          //< full duplex

/*
    ethernet link status callback

    @param[in] sts          new link status
                            - @b NVTETH_LINK_DOWN
                            - @b NVTETH_LINK_UP

    @return void
*/
//@{
typedef void (*NVTETH_LINK_CB)(unsigned int sts);

void nvteth_register_link_cb(NVTETH_LINK_CB fp);

#endif
