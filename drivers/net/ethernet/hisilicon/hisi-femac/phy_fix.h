#define HISILICON_PHY_ID_FESTAV272     0x20669901
#define HISILICON_PHY_ID_FESTAV115     0x20669903
#define HISILICON_PHY_MASK             0xffffffff

#define MII_EXPMD 0x1d
#define MII_EXPMA 0x1e

void phy_register_fixups(void);
void phy_unregister_fixups(void);
