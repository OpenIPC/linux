#include "hal_clk.h"
#include "hal_debug.h"

#define RIU_BASE_ADDR   (0x1F000000)
#define BANK_CAL(addr)  ((addr<<9) + (RIU_BASE_ADDR))

#define BANK_CLK_GEN    (BANK_CAL(0x100A))

#if (HAL_MSG_LEVL < HAL_MSG_DBG)
#define REGR(base,idx)      ms_readw(((uint)base+(idx)*4))
#define REGW(base,idx,val)  ms_writew(val,((uint)base+(idx)*4))
#else
#define REGR(base,idx)      ms_readw(((uint)base+(idx)*4))
#define REGW(base,idx,val)  do{HAL_MSG(HAL_MSG_DBG, "write 0x%08X = 0x%04X\n", ((uint)base+(idx)*4), val); ms_writew(val,((uint)base+(idx)*4));} while(0)
#endif


/*******************************************************************************************************************
 * clk_hal_init
 *   init device clock
 *
 * Parameters:
 *   RIU_BASE_ADDR:  clock base address
 *
 * Return:
 *   0: OK, othes: failed
 */
//<MStar Software>//CEVA PLL: 600MHz setting
int ive_clk_hal_init(void)
{
    u16 value;

    value = REGR(BANK_CLK_GEN, 0x52);
    value &= ~0xF8;
    value |= 0x80; //320 MHz

    REGW(BANK_CLK_GEN, 0x52, value);

    return 0;
}
