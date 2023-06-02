/*
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       heavyload.h

    @author     Novatek FW Team
    @date       2019/07/8
*/
#ifndef _HEAVYLOAD_H
#define _HEAVYLOAD_H


extern int  heavyload_cmd_showhelp(int (*dump)(const char *fmt, ...));
extern int  heavyload_cmd_execute(unsigned char argc, char **argv);
#endif