
#include <h2v3_def.h>
#include <h2v3_reg.h>

#define _BIT(sb)    ((2U<<(1?sb))-(1U<<(0?sb)))
struct field_t {
    unsigned int lsb: 5;
    unsigned int loc:11;
    unsigned int len: 8;
    unsigned int io:  2;
    unsigned int tr:  6;
    unsigned int msk;
}
fields[] =
{
#define H2REG(name,pos,sb,trace,rw,cmm)    {.loc=pos,.lsb=0?sb,.len=(1?sb)-(0?sb)+1,.io=rw,.tr=trace,.msk=_BIT(sb)}
#include <h2v3_regtab.h>
};

int H2PutRegVal(uint* mir, enum H2REG_INDEX idx, uint  val)
{
    const struct field_t* fld;
    if ((unsigned)idx >= HWIF_ENC_ENTRIES)
        return -1;
    fld = &fields[idx];
    mir[fld->loc] &=~fld->msk;
    mir[fld->loc] |= fld->msk&(val<<fld->lsb);
    return 0;
}

int H2GetRegVal(uint* mir, enum H2REG_INDEX idx, uint* val)
{
    uint reg;
    const struct field_t* fld;
    if ((unsigned)idx >= HWIF_ENC_ENTRIES)
        return -1;
    fld = &fields[idx];
    reg = fld->msk&mir[fld->loc];
    if (val)
    {
       *val = reg>>fld->lsb;
        return 0;
    }
    return -1;
}
