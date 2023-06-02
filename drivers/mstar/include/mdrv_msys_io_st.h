#ifndef _MDRV_MSYS_IO_ST_H_
#define _MDRV_MSYS_IO_ST_H_

/*******************************************************/
#define IOCTL_MSYS_VERSION    0x0100
/*******************************************************/

typedef struct
{
    unsigned int VerChk_Version;
    char name[16];
    unsigned int length;       //32 bit
    unsigned long long phys;   //64 bit
    unsigned long long kvirt; //Kernel Virtual Address 64 bit
    unsigned int option; //reserved
    unsigned int VerChk_Size;
} __attribute__ ((__packed__)) MSYS_DMEM_INFO;

typedef struct
{
    unsigned int VerChk_Version;
    unsigned int VerChk_Size;
} __attribute__ ((__packed__)) MSYS_DUMMY_INFO;

typedef struct
{
    unsigned int VerChk_Version;
    unsigned long long addr;
    unsigned int VerChk_Size;
} __attribute__ ((__packed__)) MSYS_ADDR_TRANSLATION_INFO;

typedef struct
{
    unsigned int VerChk_Version;
    unsigned long long addr;
    unsigned int size;
    unsigned int VerChk_Size;
} __attribute__ ((__packed__)) MSYS_MMIO_INFO;

typedef struct
{
    unsigned int VerChk_Version;
    unsigned long long phys;   //64 bit
    unsigned int length;       //32 bit
    unsigned char id[16];
    unsigned int r_protect;
    unsigned int w_protect;
    unsigned int inv_protect;
    unsigned int VerChk_Size;
} __attribute__ ((__packed__)) MSYS_MIU_PROTECT_INFO;

typedef struct
{
    unsigned int VerChk_Version;
    unsigned char str[32];
    unsigned int VerChk_Size;
} __attribute__ ((__packed__)) MSYS_STRING_INFO;

typedef struct
{
    unsigned int VerChk_Version;
    unsigned int temp;
    unsigned int VerChk_Size;
} __attribute__ ((__packed__)) MSYS_TEMP_INFO;

typedef struct
{
    unsigned int VerChk_Version;
    unsigned long long udid;
    unsigned int VerChk_Size;
} __attribute__ ((__packed__)) MSYS_UDID_INFO;

typedef struct
{
	unsigned int VerChk_Version;
    char name[16];
    unsigned int length;       //32 bit
    unsigned long long kphy_src;   //Kernel Virtual Address src 64 bit
    unsigned long long kphy_des; //Kernel Virtual Address des64 bit
    unsigned int VerChk_Size;
} __attribute__ ((__packed__)) MSYS_DMA_INFO;

typedef struct
{
    char name[128];
    unsigned long size;
    void *handle; //don't set this
    void *parent;
    void *data;
} MSYS_PROC_DEVICE;

typedef struct
{
  void *handle;
  char name[128];
  unsigned int type;
  unsigned int offset;
} MSYS_PROC_ATTRIBUTE;

typedef enum
{
    MSYS_PROC_ATTR_CHAR,
    MSYS_PROC_ATTR_UINT,
    MSYS_PROC_ATTR_INT,   //also for enum, bool
    MSYS_PROC_ATTR_XINT,
    MSYS_PROC_ATTR_ULONG,
    MSYS_PROC_ATTR_LONG,
    MSYS_PROC_ATTR_XLONG,
    MSYS_PROC_ATTR_ULLONG,
    MSYS_PROC_ATTR_LLONG,
    MSYS_PROC_ATTR_XLLONG,
    MSYS_PROC_ATTR_STRING,
} MSYS_PROC_ATTR_ENUM;

#endif
