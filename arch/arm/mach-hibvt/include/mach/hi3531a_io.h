#ifndef __HI3531A_IO_H
#define __HI3531A_IO_H

#ifdef CONFIG_PCI
#define IO_SPACE_LIMIT  0xFFFFFFFF

#define __io(a)         __typesafe_io(a)
#define __mem_pci(a)    (a)
#endif

/* phys_addr        virt_addr
 * 0x1000_0000 <-----> 0xFE00_0000
 * 0x1071_0000 <-----> 0xFE71_0000
 */
#define HI3531A_IOCH1_VIRT  (0xFE000000)
#define HI3531A_IOCH1_PHYS  (0x10000000)
#define HI3531A_IOCH1_SIZE  (0x00710000)
#define IO_OFFSET_IOCH1     (0xEE000000)

/* phys_addr        virt_addr
 * 0x1100_0000 <-----> 0xFE78_0000
 * 0x1104_0000 <-----> 0xFE7C_0000
 */
#define HI3531A_IOCH2_VIRT  (0xFE780000)
#define HI3531A_IOCH2_PHYS  (0x11000000)
#define HI3531A_IOCH2_SIZE  (0x00040000)
#define IO_OFFSET_IOCH2     (0xED780000)

/* phys_addr        virt_addr
 * 0x1200_0000 <-----> 0xFE80_0000
 * 0x122F_0000 <-----> 0xFEAF_0000
 */
#define HI3531A_IOCH3_VIRT  (0xFE800000)
#define HI3531A_IOCH3_PHYS  (0x12000000)
#define HI3531A_IOCH3_SIZE  (0x002F0000)
#define IO_OFFSET_IOCH3     (0xEC800000)

/* phys_addr        virt_addr
 * 0x1300_0000 <-----> 0xFEB0_0000
 * 0x131A_0000 <-----> 0xFECA_0000
 */
#define HI3531A_IOCH4_VIRT  (0xFEB00000)
#define HI3531A_IOCH4_PHYS  (0x13000000)
#define HI3531A_IOCH4_SIZE  (0x001A0000)
#define IO_OFFSET_IOCH4     (0xEBB00000)

#define IO_ADDR_HIGH_H(x)   ((x) + IO_OFFSET_IOCH4)
#define IO_ADDR_HIGH_L(x)   ((x) + IO_OFFSET_IOCH3)
#define IO_ADDR_LOW_H(x)    ((x) + IO_OFFSET_IOCH2)
#define IO_ADDR_LOW_L(x)    ((x) + IO_OFFSET_IOCH1)

#define __IO_ADDR_HIGH(x) ((x >= HI3531A_IOCH4_PHYS) ? IO_ADDR_HIGH_H(x) \
        : IO_ADDR_HIGH_L(x))

#define __IO_ADDR_LOW(x) ((x >= HI3531A_IOCH2_PHYS) ? IO_ADDR_LOW_H(x) \
        : IO_ADDR_LOW_L(x))

#define IO_ADDRESS(x)   ((x) >= HI3531A_IOCH3_PHYS ? __IO_ADDR_HIGH(x) \
        : __IO_ADDR_LOW(x))

#define __io_address(n) (IOMEM(IO_ADDRESS(n)))

#endif /* __HI3531A_IO_H */
