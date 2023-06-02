#ifndef __FH_CHIPID_H__
#define __FH_CHIPID_H__

#define FH_CHIP_FH8830          0x883000A1
#define FH_CHIP_FH8630M         0x883000B1
#define FH_CHIP_FH8632          0x863200A1
#define FH_CHIP_FH8632v2        0x863200A2
#define FH_CHIP_FH8856          0x885600A1
#define FH_CHIP_FH8852          0x885600B1
#define FH_CHIP_FH8626V100      0x8626A100
#define FH_CHIP_FH8852V200      0x8852A200
#define FH_CHIP_FH8856V200      0x8856A200
#define FH_CHIP_FH8858V200      0x8858A200
#define FH_CHIP_FH8856V201      0x8856A201
#define FH_CHIP_FH8858V201      0x8858A201
#define FH_CHIP_FH8852V210      0x8852A210
#define FH_CHIP_FH8856V210      0x8856A210
#define FH_CHIP_FH8858V210      0x8858A210
#define FH_CHIP_FH8652          0x8652A100
#define FH_CHIP_FH8656          0x8656A100
#define FH_CHIP_FH8658          0x8658A100

struct fh_chip_info
{
    int _plat_id; /* 芯片寄存器中的plat_id */
    int _chip_id; /* 芯片寄存器中的chip_id */
    int _chip_mask; /* 芯片寄存器中的chip_id */
    int chip_id; /* 芯片chip_id，详见上述定义 */
    int ddr_size; /* 芯片DDR大小，单位Mbit */
    char chip_name[32]; /* 芯片名称 */
};

void fh_get_chipid(unsigned int *plat_id, unsigned int *chip_id);
unsigned int fh_get_ddrsize_mbit(void);
char *fh_get_chipname(void);
struct fh_chip_info *fh_get_chip_info(void);

unsigned int fh_is_8830(void);
unsigned int fh_is_8632(void);
unsigned int fh_is_8852(void);
unsigned int fh_is_8856(void);
unsigned int fh_is_8626v100(void);
unsigned int fh_is_8852v200(void);
unsigned int fh_is_8856v200(void);
unsigned int fh_is_8858v200(void);
unsigned int fh_is_8856v201(void);
unsigned int fh_is_8858v201(void);
unsigned int fh_is_8852v210(void);
unsigned int fh_is_8856v210(void);
unsigned int fh_is_8858v210(void);
unsigned int fh_is_8652(void);
unsigned int fh_is_8656(void);
unsigned int fh_is_8658(void);

int fh_chipid_init(void);

#endif
