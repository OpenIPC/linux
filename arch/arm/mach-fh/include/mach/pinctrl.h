#ifndef PINCTRL_H_
#define PINCTRL_H_
#include "pinctrl_osdep.h"
#include <linux/seq_file.h>

#define PINCTRL_UNUSED      (-1)

#define PUPD_NONE           (0)
#define PUPD_UP             (1)
#define PUPD_DOWN           (2)

#define INPUT_DISABLE       (0)
#define INPUT_ENABLE        (1)
#define OUTPUT_DISABLE      (0)
#define OUTPUT_ENABLE       (1)

#define FUNC0               (0)
#define FUNC1               (1)
#define FUNC2               (2)
#define FUNC3               (3)
#define FUNC4               (4)
#define FUNC5               (5)
#define FUNC6               (6)
#define FUNC7               (7)

#define NEED_CHECK_PINLIST  (1)

#define MAX_FUNC_NUM    8

#define PINCTRL_FUNC(name, id, sel, pupd)           \
PinCtrl_Pin PAD##id##_##name =                      \
{                                                   \
    .pad_id         = id,                           \
    .func_name      = #name,                        \
    .reg_offset     = (id * 4),                     \
    .func_sel       = sel,                          \
    .pullup_pulldown= pupd,			    \
}

#define PINCTRL_MUX(pname, sel, ...)                \
PinCtrl_Mux MUX_##pname =                           \
{                                                   \
    .mux_pin = { __VA_ARGS__ },                     \
    .cur_pin = sel,                                 \
}

#define PINCTRL_DEVICE(name, count, ...)            \
typedef struct                                      \
{                                                   \
    char *dev_name;                                 \
    int mux_count;                                  \
    OS_LIST list;                                   \
    PinCtrl_Mux *mux[count];                        \
} PinCtrl_Device_##name;                            \
PinCtrl_Device_##name pinctrl_dev_##name =          \
{                                                   \
     .dev_name = #name,                             \
     .mux_count = count,                            \
     .mux = { __VA_ARGS__ },                        \
}

typedef union {
    struct
    {
        unsigned int        : 12;   //0~11
        unsigned int ie     : 1;    //12
        unsigned int        : 3;    //13~15
        unsigned int pdn    : 1;    //16
        unsigned int        : 3;    //17~19
        unsigned int pun    : 1;    //20
        unsigned int        : 3;    //21~23
        unsigned int mfs    : 4;    //24~27
        unsigned int oe     : 1;    //28
        unsigned int        : 3;    //29~31
    } bit;
    unsigned int dw;
} PinCtrl_Register;

typedef struct
{
	char *func_name;
	PinCtrl_Register *reg;
	int pad_id			: 12;
	unsigned int reg_offset		: 12;
	int func_sel			: 4;
	int input_enable		: 1;
	int output_enable		: 1;
	int pullup_pulldown		: 2;
}PinCtrl_Pin;

typedef struct
{
    //todo: int lock;
    int cur_pin;
    PinCtrl_Pin *mux_pin[MUX_NUM];
} PinCtrl_Mux;

typedef struct
{
    void *vbase;
    void *pbase;
    PinCtrl_Pin *pinlist[PAD_NUM];
} PinCtrl_Object;

typedef struct
{
    char *dev_name;
    int mux_count;
    OS_LIST list;
    void *mux;
}PinCtrl_Device;

void fh_pinctrl_init(unsigned int base);
void fh_pinctrl_prt(struct seq_file *sfile);
int fh_pinctrl_smux(char *devname, char* muxname, int muxsel, unsigned int flag);
int fh_pinctrl_sdev(char *devname, unsigned int flag);
void fh_pinctrl_init_devicelist(OS_LIST *list);
#endif /* PINCTRL_H_ */
