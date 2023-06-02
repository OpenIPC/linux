#ifndef __UNFD_DEVICE_H__
#define __UNFD_DEVICE_H__

#include "drvNAND.h"
#if IF_IP_VERIFY
#if defined(H9LA25G25HAMBR) && H9LA25G25HAMBR
	#define NAND_DEVICE_ID               {0xAD,0x35}
	#define NAND_DEVICE_ID_LEN           2
	#define NAND_BLK_CNT                 0x800
	#define NAND_BLK_PAGE_CNT            0x20

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C3TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C2TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x200
	#define NAND_SPARE_BYTE_CNT          0x10
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(H8ACU0CE0DAR) && H8ACU0CE0DAR
	#define NAND_DEVICE_ID               {0xAD,0x76}
	#define NAND_DEVICE_ID_LEN           2
	#define NAND_BLK_CNT                 0x1000
	#define NAND_BLK_PAGE_CNT            0x20

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C4TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C3TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x200
	#define NAND_SPARE_BYTE_CNT          0x10
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(TY8A0A111178KC4) && TY8A0A111178KC4
	#define NAND_DEVICE_ID               {0x98,0xC1,0x90,0x55,0x76,0x14}
	#define NAND_DEVICE_ID_LEN           6
	#define NAND_BLK_CNT                 0x400
	#define NAND_BLK_PAGE_CNT            0x40

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C4TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C2TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(TY890A111229KC40) && TY890A111229KC40
	#define NAND_DEVICE_ID               {0x98,0xD0,0x00,0x15,0x72,0x14}
	#define NAND_DEVICE_ID_LEN           6
	#define NAND_BLK_CNT                 0x200
	#define NAND_BLK_PAGE_CNT            0x40

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C4TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C2TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(TY8A0A111162KC40) && TY8A0A111162KC40 
	#define NAND_DEVICE_ID               {0x98,0xA1,0x90,0x15,0x76,0x14}
	#define NAND_DEVICE_ID_LEN           6
	#define NAND_BLK_CNT                 0x400
	#define NAND_BLK_PAGE_CNT            0x40

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C4TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C2TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(K9F1G08X0C) && K9F1G08X0C /* K9F1G08U0C, K9F1G08B0C */
	#define NAND_DEVICE_ID               {0xEC,0xF1,0x00,0x95,0x40}
	#define NAND_DEVICE_ID_LEN           5
	#define NAND_BLK_CNT                 0x400
	#define NAND_BLK_PAGE_CNT            0x40

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C4TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C2TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(K522H1GACE) && K522H1GACE /* K522H1GACE, K522H1GACE */
	#define NAND_DEVICE_ID               {0xEC,0xBA,0x00,0x55,0x44}
	#define NAND_DEVICE_ID_LEN           5

	#define NAND_BLK_CNT                 0x800
	#define NAND_BLK_PAGE_CNT            0x40

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C5TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C3TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(H27UAG8T2M) && H27UAG8T2M
	#define NAND_DEVICE_ID               {0xAD,0xD5,0x14,0xB6,0x44}
	#define NAND_DEVICE_ID_LEN           5

	#define NAND_BLK_CNT                 0x1000
	#define NAND_BLK_PAGE_CNT            0x80

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C5TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C3TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x1000
	#define NAND_SPARE_BYTE_CNT          0x80
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(H27UAG8T2M_FAKE2K) && H27UAG8T2M_FAKE2K
	#define NAND_DEVICE_ID               {0xAD,0xD5,0x14,0xB6,0x44}
	#define NAND_DEVICE_ID_LEN           5

	#define NAND_BLK_CNT                 0x1000
	#define NAND_BLK_PAGE_CNT            0x80

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C5TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C3TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_24BIT1KB

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x80
	#define NAND_SECTOR_BYTE_CNT         0x400

#elif defined(NAND256W3A) && NAND256W3A
	#define NAND_DEVICE_ID               {0x20,0x75}
	#define NAND_DEVICE_ID_LEN           2

	#define NAND_BLK_CNT                 0x800
	#define NAND_BLK_PAGE_CNT            0x20

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C3TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C2TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x200
	#define NAND_SPARE_BYTE_CNT          0x10
	#define NAND_SECTOR_BYTE_CNT         0x200


#elif defined(NAND512W3A2C) && NAND512W3A2C
	#define NAND_DEVICE_ID               {0x20,0x76}
	#define NAND_DEVICE_ID_LEN           2

	#define NAND_BLK_CNT                 0x1000
	#define NAND_BLK_PAGE_CNT            0x20

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C4TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C3TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x200
	#define NAND_SPARE_BYTE_CNT          0x10
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(H27UF081G2A) && H27UF081G2A
	#define NAND_DEVICE_ID               {0xAD,0xF1,0x80,0x1D}
	#define NAND_DEVICE_ID_LEN           4

	#define NAND_BLK_CNT                 0x400
	#define NAND_BLK_PAGE_CNT            0x40

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C4TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C2TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(H27UBG8T2A) && H27UBG8T2A
	#define NAND_DEVICE_ID               {0xAD,0xD7,0x94,0x9A,0x74,0x42}
	#define NAND_DEVICE_ID_LEN           6

	#define NAND_BLK_CNT                 0x1000
	#define NAND_BLK_PAGE_CNT            0x100

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C5TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C3TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_24BIT1KB

	#define NAND_PAGE_BYTE_CNT           0x2000
	#define NAND_SPARE_BYTE_CNT          448
	#define NAND_SECTOR_BYTE_CNT         0x400

#elif defined(H27U1G8F2B) && H27U1G8F2B
	#define NAND_DEVICE_ID               {0xAD,0xF1,0x00,0x15}
	#define NAND_DEVICE_ID_LEN           4

	#define NAND_BLK_CNT                 0x400
	#define NAND_BLK_PAGE_CNT            0x40

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C4TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C2TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(HY27UF084G2B) && HY27UF084G2B
	#define NAND_DEVICE_ID               {0xAD,0xDC,0x10,0x95,0x54}
	#define NAND_DEVICE_ID_LEN           5

	#define NAND_BLK_CNT                 0x1000
	#define NAND_BLK_PAGE_CNT            0x40

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C5TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C3TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(HY27UF082G2B) && HY27UF082G2B
	//#define NAND_DEVICE_ID               "\xAD\xDA\x10\x95\x44"
	#define NAND_DEVICE_ID               {0xAD,0xDA,0x10,0x95,0x44}
	#define NAND_DEVICE_ID_LEN           5

	#define NAND_BLK_CNT                 0x800
	#define NAND_BLK_PAGE_CNT            0x40

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C5TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C3TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200
	
#elif defined(K9HAG08U1M) && K9HAG08U1M
	#define NAND_DEVICE_ID               {0xEC,0xD3,0x55,0x25,0x58}
	#define NAND_DEVICE_ID_LEN           5

	#define NAND_BLK_CNT                 0x1000
	#define NAND_BLK_PAGE_CNT            0x80

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C5TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C3TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(K511F12ACA) && K511F12ACA
	#define NAND_DEVICE_ID               {0xEC,0xA1,0x00,0x15,0x40}
	#define NAND_DEVICE_ID_LEN           5

	#define NAND_BLK_CNT                 0x400
	#define NAND_BLK_PAGE_CNT            0x40

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C4TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C2TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_RS

	#define NAND_PAGE_BYTE_CNT           0x800
	#define NAND_SPARE_BYTE_CNT          0x40
	#define NAND_SECTOR_BYTE_CNT         0x200

#elif defined(P1UAGA30AT) && P1UAGA30AT
	#define NAND_DEVICE_ID               {0xC8,0xD5,0x14,0x29,0x34,0x01}
	#define NAND_DEVICE_ID_LEN           0x6

	#define NAND_BLK_CNT                 0x1000
	#define NAND_BLK_PAGE_CNT            0x80

	#define NAND_RW_ADR_CYCLE_CNT        ADR_C5TFS0
	#define NAND_E_ADR_CYCLE_CNT         ADR_C3TRS0
	#define NAND_ECC_TYPE                ECC_TYPE_12BIT

	#define NAND_PAGE_BYTE_CNT           0x1000
	#define NAND_SPARE_BYTE_CNT          218
	#define NAND_SECTOR_BYTE_CNT         0x200
#else
	#error "No NAND device specified\n"
#endif
#endif // IF_IP_VERIFY
#endif /* __UNFD_DEVICE_H__ */
