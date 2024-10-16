#include <linux/types.h>
int rtk_mdio_read(u32 len, u8 phy_adr, u8 reg, u32 *value);
int rtk_mdio_write(u32 len, u8 phy_adr, u8 reg, u32 data);
