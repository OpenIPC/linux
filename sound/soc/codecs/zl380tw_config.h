#ifndef __ZL380TW_CONFIG_H_
#define __ZL380TW_CONFIG_H_

#define ZL380XX_CFG_BLOCK_SIZE 1
typedef struct {
   unsigned short reg;   /*the register */
   unsigned short value[ZL380XX_CFG_BLOCK_SIZE]; /*the value to write into reg */
} dataArr;

extern const unsigned short configStreamLen;
extern const dataArr st_twConfig[];
extern const unsigned short zl_configBlockSize;
#endif
