#ifndef __HI3519AV100_IO_H
#define __HI3519AV100_IO_H

#ifdef CONFIG_PCI
#define IO_SPACE_LIMIT  0xFFFFFFFF

#define __io(a)         __typesafe_io(a)
#define __mem_pci(a)    (a)
#endif

#endif
