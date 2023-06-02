
#include <vhe_defs.h>
#include <vhe_regs.h>

typedef struct vhe_regs {
    uint    msk;
    uint8   lsb, idx, tr, wr;
} vhe_regs;

const vhe_regs vhe_regs_list[] = {
#define _VHE_REGS_TABLE_
#include <vhe_regs_table.h>
#undef  _VHE_REGS_TABLE_
};

int
vhe_put_regs(
    uint*       regs,
    vhe_field   fld,
    uint        val)
{
    const vhe_regs *field;

    if ((unsigned)fld >= HWIF_ENTRIES)
    {
        return -1;
    }

    field = &vhe_regs_list[fld];

    regs[field->idx] &= ~field->msk;
    regs[field->idx] |= (val<<field->lsb) & field->msk;
    return 0;
}

int
vhe_get_regs(
    uint*       regs,
    vhe_field   fld,
    uint*       val)
{
    const vhe_regs *field;
    uint regv;

    if ((unsigned)fld >= HWIF_ENTRIES || !val)
    {
        return -1;
    }

    field = &vhe_regs_list[fld];

    regv = regs[field->idx] & field->msk;
    *val = regv >> field->lsb;
    return 0;
}
