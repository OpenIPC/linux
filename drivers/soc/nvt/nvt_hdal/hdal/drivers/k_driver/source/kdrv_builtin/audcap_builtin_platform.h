#ifndef _AUDCAP_BUILTIN_PLATFORM_H
#define _AUDCAP_BUILTIN_PLATFORM_H

extern void _audcap_builtin_set_clk(int sampling_rate);
extern void _audcap_builtin_set_pad(void);
extern int audcap_builtin_get_samplerate(void);
extern int audcap_builtin_get_channel(void);
extern int audcap_builtin_get_bufcount(void);
extern int audcap_builtin_get_bufsamplecnt(void);
extern int audcap_builtin_get_rec_src(void);
extern int audcap_builtin_get_vol(void);
extern UINT32 audcap_builtin_get_aec_en(void);
extern UINT32 audcap_builtin_get_txchannel(void);
extern UINT32 audcap_builtin_get_default_setting(void);

#endif