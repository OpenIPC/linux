/**
    Image operation module.

    This module supports for rtos initialization

    @file       kflow.h

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _KFLOW_H_
#define _KFLOW_H_

extern int kflow_videoenc_init(void);
extern int kflow_videodec_init(void);
extern int kflow_videoproc_init(void);
extern int kflow_videocap_init(void);
extern int kflow_videoout_init(void);
extern int kflow_audiocap_init(void);
extern int kflow_audioout_init(void);
extern int kflow_audiodec_init(void);
extern int kflow_audioenc_init(void);
extern int kflow_ctl_ipp_init(void);

#endif //_KFLOW_H_