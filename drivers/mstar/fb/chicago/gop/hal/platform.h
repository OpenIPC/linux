#ifndef	__PLATFORM_H__
#define	__PLATFORM_H__

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define MIU0_BUS_ADDR           0x40000000
#define MIU1_MASK               0x00000000 //use (ADDR & MIU1_MASK) to check if ADDR is in MIU1 after calling ms_mem_bus_to_MIU()


#define IO_PHYS         		0x1F000000
#define IO_OFFSET       		0xDE000000
#define IO_SIZE         		0x00400000
#define IO_VIRT         		(IO_PHYS + IO_OFFSET)
#define io_p2v(pa)      		((pa) + IO_OFFSET)
#define io_v2p(va)      		((va) - IO_OFFSET)
//#define IO_ADDRESS(x)   		io_p2v((unsigned int)x)


#define PERI_PHYS         		0x16000000
#define PERI_OFFSET       		0xE6000000
#define PERI_SIZE         		0x2000
#define PERI_VIRT         		(PERI_PHYS + PERI_OFFSET)
#define peri_p2v(pa)      		((pa) + PERI_OFFSET)
#define peri_v2p(va)      		((va) - PERI_OFFSET)
#define PERI_ADDRESS(x)   		peri_p2v(x)

#define PERI_W(a,v)				(*(volatile unsigned int *)PERI_ADDRESS(a)=v)
#define PERI_R(a)				(*(volatile unsigned int *)PERI_ADDRESS(a))

#define SCU_PHYS                (PERI_PHYS)

#define L2_CACHE_PHYS			0x15000000
#define L2_CACHE_OFFSET       	0xE7003000
#define L2_CACHE_SIZE         	0x1000
#define L2_CACHE_VIRT         	(L2_CACHE_PHYS + L2_CACHE_OFFSET)
#define L2_CACHE_p2v(pa)      	((pa) + L2_CACHE_OFFSET)
#define L2_CACHE_v2p(va)      	((va) - L2_CACHE_OFFSET)
#define L2_CACHE_ADDRESS(x)   	L2_CACHE_p2v(x)

#define L2_CACHE_write(v,a)         (*(volatile unsigned int *)L2_CACHE_ADDRESS(a) = (v))
#define L2_CACHE_read(a)            (*(volatile unsigned int *)L2_CACHE_ADDRESS(a))


#define IMI_PHYS         		0xE0000000
#define IMI_OFFSET       		0x1C005000
#define IMI_SIZE         		0x50000
#define IMI_VIRT         		(IMI_PHYS + IMI_OFFSET)
#define imi_p2v(pa)      		((pa) + IMI_OFFSET)
#define imi_v2p(va)      		((va) - IMI_OFFSET)
#define IMI_ADDRESS(x)   		imi_p2v((unsigned int)x)




// Register macros
#define GET_REG_ADDR(x, y)  ((x) + ((y) << 2))


/* read register by byte */
#define ms_readb(a) (*(volatile unsigned char *)IO_ADDRESS(a))

/* read register by word */
#define ms_readw(a) (*(volatile unsigned short *)IO_ADDRESS(a))

/* read register by long */
#define ms_readl(a) (*(volatile unsigned int *)IO_ADDRESS(a))

/* write register by byte */
#define ms_writeb(v,a) (*(volatile unsigned char *)IO_ADDRESS(a) = (v))

/* write register by word */
#define ms_writew(v,a) (*(volatile unsigned short *)IO_ADDRESS(a) = (v))

/* write register by long */
#define ms_writel(v,a) (*(volatile unsigned int *)IO_ADDRESS(a) = (v))


//------------------------------------------------------------------------------
//
//  Macros:  INREGx/OUTREGx/SETREGx/CLRREGx
//
//  This macros encapsulates basic I/O operations.
//  Memory address space operation is used on all platforms.
//
#define INREG8(x)           ms_readb(x)
#define OUTREG8(x, y)       ms_writeb((u8)(y), x)
#define SETREG8(x, y)       OUTREG8(x, INREG8(x)|(y))
#define CLRREG8(x, y)       OUTREG8(x, INREG8(x)&~(y))
#define INREGMSK8(x, y)     (INREG8(x) & (y))
#define OUTREGMSK8(x, y, z) OUTREG8(x, ((INREG8(x)&~(z))|((y)&(z))))

#define INREG16(x)              ms_readw(x)
#define OUTREG16(x, y)          ms_writew((u16)(y), x)
#define SETREG16(x, y)          OUTREG16(x, INREG16(x)|(y))
#define CLRREG16(x, y)          OUTREG16(x, INREG16(x)&~(y))
#define INREGMSK16(x, y)        (INREG16(x) & (y))
#define OUTREGMSK16(x, y, z)    OUTREG16(x, ((INREG16(x)&~(z))|((y)&(z))))

#define INREG32(x)              ms_readl(x)
#define OUTREG32(x, y)          ms_writel((u32)(y), x)
#define SETREG32(x, y)          OUTREG32(x, INREG32(x)|(y))
#define CLRREG32(x, y)          OUTREG32(x, INREG32(x)&~(y))
#define INREGMSK32(x, y)        (INREG32(x) & (y))
#define OUTREGMSK32(x, y, z)    OUTREG32(x, ((INREG32(x)&~(z))|((y)&(z))))


#define PM_READ32(x)  		((((unsigned int)INREG16(x+4)<<16)&0xFFFF0000) | (INREG16(x) & 0x0000FFFF))
#define PM_WRITE32(x,y)      do{OUTREG16(x,(y&0x0000FFFF));OUTREG16((x+4),((y>>16)&0x0000FFFF));}while(0)

#define XTAL_26000K        26000000
#define XTAL_24000K        24000000
#define XTAL_16369K        16369000
#define XTAL_16367K        16367000

#define CPU_ID_CORE0       0x0
#define CPU_ID_CORE1       0x1


//------------------------------------------------------------------------------
//
//  Define:  MSBASE_REG_RIU_PA
//
//  Locates the RIU register base.
//
#define MS_BASE_REG_RIU_PA				IO_PHYS



extern unsigned long long ms_mem_bus_to_phys(unsigned long long x);
extern unsigned long long ms_mem_phys_to_bus(unsigned long long x);
extern unsigned long long ms_mem_bus_to_MIU(unsigned long long x);
extern unsigned long long ms_mem_MIU_to_bus(unsigned long long x);

typedef enum
{
	DEVINFO_BOOT_TYPE_SPI=0x01,
	DEVINFO_BOOT_TYPE_EMMC=0x02,
	DEVINFO_BOOT_TYPE_8051=0x03,
}DEVINFO_BOOT_TYPE;

typedef enum
{
    DEVINFO_PANEL_070_DEFAULT    = 0x0,
    DEVINFO_PANEL_HSD_070I_DW1   = 0x01,
    DEVINFO_PANEL_HSD_070P_FW3   = 0x02,
    DEVINFO_PANEL_AT_102_03TT91  = 0x03,
    DEVINFO_PANEL_HSD_062I_DW1   = 0x04,
    DEVINFO_PANEL_HSD_070I_DW2   = 0x05,
    DEVINFO_PANEL_HSD_LVDS_800480= 0x06,
    DEVINFO_PANEL_N070ICG_LD1     =0x08,
    DEVINFO_PANEL_HSD_062I_DW2   = 0x09,
    DEVINFO_PANEL_HSD_070I_MODE2 = 0x0a,
    E_PANEL_UNKNOWN           = 0xFF
}DEVINFO_PANEL_TYPE;


typedef enum
{
    DEVINFO_BD_MST154A_D01A_S     = 0x0801,
    DEVINFO_BD_MST786_SZDEMO      = 0x0802,
    DEVINFO_BD_MST786_CUSTOM10    = 0x0810,
    DEVINFO_BD_MST786_CUSTOM20    = 0x0820,
    DEVINFO_BD_MST786_CUSTOM30    = 0x0830,
    E_BD_UNKNOWN           = 0xFFFF
}DEVINFO_BOARD_TYPE;

typedef enum
{
    DEVINFO_RTK_FLAG_0     = 0x0,
    DEVINFO_RTK_FLAG_1     = 0x01,
    E_RTK_UNKNOWN           = 0xFFFF
}DEVINFO_RTK_FLAG;

typedef struct
{
	char name[16];
    unsigned long start; //MIU Address
    unsigned long length;
    unsigned long flag0;
    unsigned long flag1; //Kernel Virtual Address
} MS_MEM_INFO;


#endif
