/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nvtmpp_debug_cmd.h

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2017/02/13
*/
#ifndef _NVTMPP_DEBUG_CMD_H
#define _NVTMPP_DEBUG_CMD_H

extern void nvtmpp_install_cmd(void);
extern void nvtmpp_uninstall_cmd(void);
extern int  nvtmpp_cmd_showhelp(int (*dump)(const char *fmt, ...));
extern int  nvtmpp_cmd_execute(unsigned char argc, char **argv);
#endif

