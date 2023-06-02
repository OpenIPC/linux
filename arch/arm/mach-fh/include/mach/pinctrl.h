#ifndef PINCTRL_H_
#define PINCTRL_H_
#include "pinctrl_osdep.h"
#include <linux/seq_file.h>

#define MSC_3_3V			(0)
#define MSC_1_8V			(1)

#define PINCTRL_UNUSED      (-1)

#define PUPD_NONE           (0)
#define PUPD_UP             (1)
#define PUPD_DOWN           (2)

#define INPUT_DISABLE       (0)
#define INPUT_ENABLE        (1)
#define OUTPUT_DISABLE      (0)
#define OUTPUT_ENABLE       (1)

#if defined(CONFIG_MACH_FH8626V100)
#define PUPD_DISABLE        (1)
#define PUPD_ENABLE         (0)
#define PUPD_ZERO           (0)
#else
#define PUPD_DISABLE        (0)
#define PUPD_ENABLE         (1)
#define PUPD_ZERO           (0)
#endif

#define FUNC0               (0)
#define FUNC1               (1)
#define FUNC2               (2)
#define FUNC3               (3)
#define FUNC4               (4)
#define FUNC5               (5)
#define FUNC6               (6)
#define FUNC7               (7)
#define FUNC8               (8)
#define FUNC9               (9)


#define NEED_CHECK_PINLIST  (1)
#define PIN_BACKUP			(1<<1)
#define PIN_RESTORE			(1<<2)

#define PINCTRL_FUNC(name, id, sel, pupd, ds)  \
PinCtrl_Pin PAD##id##_##name =      \
{                                   \
	.pad_id				= id,		\
	.func_name			= #name,	\
	.reg_offset			= (id * 4),	\
	.func_sel			= sel,		\
	.pullup_pulldown	= pupd,		\
	.driving_curr		= ds,		\
	.output_enable		= 1,		\
}

#define PARA_COUNT_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N
#define PARA_COUNT(...) \
	PARA_COUNT_N(0, ##__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define PINCTRL_MUX(pname, sel, ...)                \
struct PinCtrl_Mux_##pname                          \
{                                                   \
	int cur_pin:16;                                 \
	int mux_num:16;                                 \
	PinCtrl_Pin *mux_pin[PARA_COUNT(__VA_ARGS__)];  \
} MUX_##pname =                                     \
{                                                   \
	.mux_pin = { __VA_ARGS__ },                     \
	.cur_pin = sel,                                 \
	.mux_num = PARA_COUNT(__VA_ARGS__),             \
};

#define PINCTRL_DEVICE(name, count, ...)            \
typedef struct                                      \
{                                                   \
	char *dev_name;                                 \
	int mux_count;                                  \
	OS_LIST list;                                   \
	void *mux[count];                        \
} PinCtrl_Device_##name;                            \
PinCtrl_Device_##name pinctrl_dev_##name =          \
{                                                   \
	.dev_name = #name,                             \
	.mux_count = count,                            \
	.mux = { __VA_ARGS__ },                        \
}

typedef union {
	struct {
		unsigned int sl		: 1;	/*0*/
		unsigned int		: 3;	/*1~3*/
		unsigned int ds		: 3;	/*4~6*/
		unsigned int msc	: 1;	/*7*/
		unsigned int st		: 1;	/*8*/
		unsigned int        : 3;	/*9~11*/
		unsigned int ie     : 1;    /*12*/
		unsigned int        : 3;    /*13~15*/
		unsigned int pdn    : 1;    /*16*/
		unsigned int        : 3;    /*17~19*/
		unsigned int pun    : 1;    /*20*/
		unsigned int        : 3;	/*21~23*/
		unsigned int mfs    : 4;    /*24~27*/
		unsigned int oe     : 1;    /*28*/
		unsigned int        : 3;    /*29~31*/
    } bit;
    unsigned int dw;
} PinCtrl_Register;

typedef struct
{
	char *func_name;
	PinCtrl_Register *reg;
	unsigned int pad_id			: 8;
	unsigned int reg_offset			: 12;
	unsigned int func_sel			: 4;
	unsigned int input_enable		: 1;
	unsigned int output_enable		: 1;
	unsigned int pullup_pulldown		: 2;
	unsigned int volt_mode			: 1;
	unsigned int driving_curr		: 3;
}PinCtrl_Pin;

typedef struct
{
	int cur_pin:16;
	int mux_num:16;
	PinCtrl_Pin *mux_pin[0];
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
int fh_pinctrl_spupd(char *pin_name, unsigned int pupd);
int fh_pinctrl_sds(char *pin_name, unsigned int ds);
int fh_pinctrl_set_oe(char *pin_name, unsigned int oe);
void fh_pinctrl_init_devicelist(OS_LIST *list);
char *fh_pinctrl_smux_backup(char *devname, char *muxname, int muxsel);
char *fh_pinctrl_smux_restore(char *devname, char *muxname, int muxsel);

#endif /* PINCTRL_H_ */
