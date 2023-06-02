#ifndef __VER_JPE__
#define __VER_JPE__
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include "cam_os_wrapper.h"
#include "drv_jpe_enc.h"

#include "md5.h"
#include "util_pattern.h"

#define RED "\e[1;31m"
#define BLUE "\e[1;34m"
#define NORMAL "\e[0m"

#define JPEG_TEST_COUNT   10
#define JPEG_TEST_SUCCESS 0
#define JPEG_TEST_FAIL    -1
#define JPEG_SCALE_FACTOR 100    // valid range is 1~100

#define DEVICE_PATH "/dev/mstar_jpe0"

typedef struct KMA_t
{
    void* user_ptr;
    unsigned long miu_addr;
    unsigned long phys_addr;
    unsigned long mem_size;
} KMA_t;

static KMA_t _gKmaFileIn_1;    // used for YUV RAW data
static KMA_t _gKmaFileOut_1;    // used for JPEG encoded data
static KMA_t _gKmaFileIn_2;    // used for YUV RAW data
static KMA_t _gKmaFileOut_2;    // used for JPEG encoded data

static pthread_t _gTask1 = NULL;
static pthread_t _gTask2 = NULL;



// YUV Test Data
typedef struct HashResult_t
{
    char pName[16];
    int  size;
    char pDigest[33];
} HashResult_t;

HashResult_t _gpYUVHash[] =
{
    { "CIF.nv12", 152064, "2e32b62eeb3feea39d0046d15ada1db9" },
    { "CIF.nv21", 152064, "34cf41b049437fbb681717ccda4e2332" },
    { "CIF.yuyv", 202752, "d0b623f5f4f35f131fb2fae18d47f3a0" },
    { "CIF.yvyu", 202752, "949d22105ef7a93492467fa1dc18887b" },

    { "VGA.nv12", 460800, "e5fb3e8491eb71de307fc7318a502753" },
    { "VGA.nv21", 460800, "0cf5b6a6e29f898b2e0f027e7b6a3d59" },
    { "VGA.yuyv", 614400, "ba04b608701c9ef4e34116e46cf2a983" },
    { "VGA.yvyu", 614400, "bafe471ea8445185d6b9050d75c5012a" },

    { "1088P.nv12", 3133440, "929b9895da1ac761669b7fc5751ed8eb" },
    { "1088P.nv21", 3133440, "566276a1043b8440be166d9f417ad6eb" },
    { "1088P.yuyv", 4177920, "8f3b83841950a7b068ea81bca1b22ef8" },
    { "1088P.yvyu", 4177920, "71d63f2f73c1a55fbe5c5d4b25253f13" },

    // TODO: make new sample for 2688x1536
    { "1536P.nv12", 3133440, "wrong hash value" },
    { "1536P.nv21", 3133440, "wrong hash value" },
    { "1536P.yuyv", 4177920, "wrong hash value" },
    { "1536P.yvyu", 4177920, "wrong hash value" },
};

HashResult_t _gpJpegBinHash[] =
{
    { "CIF.nv12", 3851, "b6ad3146d0863cf22f9edc9b15e1bbb7" },
    { "CIF.nv21", 3851, "b6ad3146d0863cf22f9edc9b15e1bbb7" },
    { "CIF.yuyv", 5376, "5dd1c1ed4f618e5e094f109acfcea61b" },
    { "CIF.yvyu", 5376, "5dd1c1ed4f618e5e094f109acfcea61b" },

    { "VGA.nv12", 9380, "b07682d407fefa06867d0564db356f4b" },
    { "VGA.nv21", 9380, "b07682d407fefa06867d0564db356f4b" },
    { "VGA.yuyv", 12401, "e31c3c1bb5b01e2e23a09b38225ec742" },
    { "VGA.yvyu", 12401, "e31c3c1bb5b01e2e23a09b38225ec742" },

    { "1088P.nv12", 46752, "050e81bdacbab019af6ffa07c9c44e53" },
    { "1088P.nv21", 46752, "050e81bdacbab019af6ffa07c9c44e53" },
    { "1088P.yuyv", 58892, "2868008e81f83f436aea57556652f8a4" },
    { "1088P.yvyu", 58892, "2868008e81f83f436aea57556652f8a4" },

    { "1536P.nv12", 46752, "wrong hash value" },
    { "1536P.nv21", 46752, "wrong hash value" },
    { "1536P.yuyv", 58892, "wrong hash value" },
    { "1536P.yvyu", 58892, "wrong hash value" },
};


#define DCTSIZE2 64
u16 std_luminance_quant_tbl[DCTSIZE2] =
{
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68, 109, 103,  77,
    24,  35,  55,  64,  81, 104, 113,  92,
    49,  64,  78,  87, 103, 121, 120, 101,
    72,  92,  95,  98, 112, 100, 103,  99
};

u16 std_chrominance_quant_tbl[DCTSIZE2] =
{
    17,  18,  24,  47,  99,  99,  99,  99,
    18,  21,  26,  66,  99,  99,  99,  99,
    24,  26,  56,  99,  99,  99,  99,  99,
    47,  66,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99
};


typedef enum JPEG_TEST_CLOCK_e
{
    // clk-select = <0>; // 0: 288MHz  1: 216MHz  2: 54MHz  3: 27MHz
    eCLK_288 = 0,
    eCLK_216,
    eCLK_54,
    eCLK_27
} JPEG_TEST_CLOCK_e;


typedef enum JPEG_TEST_RESOLUTION_e
{
    eRESOLUTION_CIF = 0,
    eRESOLUTION_VGA,
    eRESOLUTION_1088P,      // 1920x1088
    eRESOLUTION_1536P,      // 2688x1536
} JPEG_TEST_RESOLUTION_e;

char *_gpResolutionName[] =
{
    "CIF",
    "VGA",
    "1080P"
};

typedef enum JPEG_TEST_FORMAT_e
{
    eNV12 = 0,
    eNV21,
    eYUYV,
    eYVYU
} JPEG_TEST_FORMAT_e;

struct bo
{
    void *ptrHead;
    void *ptr;
    size_t size;
    size_t offset;
    size_t pitch;
};


static unsigned char _gDbgMsgFlag = 0;



#if 0
unsigned int GetHash(char *pFileName, unsigned char digest[])
{
    unsigned int nSize = 0;
    md5_state_t c;

    unsigned char *pBuffer = NULL;
    FILE *pFile;

    if(!pFileName)
        printf("pFileName is NULL\n");

    pFile = fopen(pFileName, "r");
    if(pFile)
    {
        fseek(pFile, 0L, SEEK_END);
        nSize = ftell(pFile);
        fseek(pFile, 0L, SEEK_SET);

        pBuffer = malloc(nSize);

        fread(pBuffer, 1, nSize, pFile);
        fclose(pFile);

        Md5Init(&c);
        Md5Append(&c, pBuffer, nSize);
        Md5Finish(&c, digest);

        free(pBuffer);
    }
    else
    {
        printf("fopen %s fail\n", pFileName);
    }
    return 1;
}
#endif


static void _PatternRelease(struct bo *pBo)
{
    CamOsMemRelease(pBo);
}

static struct bo *
_PatternCreate(unsigned int format,
               unsigned int width, unsigned int height,
               unsigned int handles[4], unsigned int pitches[4],
               unsigned int offsets[4], UtilFillPattern_e pattern, void* pMemory)
{
    unsigned int virtual_height;
    struct bo *pBo;
    unsigned int bpp;
    void *planes[3] = { 0, };
    void *virtual;

    switch(format)
    {
        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV21:
            bpp = 8;
            break;

        case DRM_FORMAT_YUYV:
        case DRM_FORMAT_YVYU:
            bpp = 16;
            break;

        default:
            CamOsDebug("unsupported format 0x%08x\n",  format);
            return NULL;
    }

    switch(format)
    {
        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV21:
            virtual_height = height * 3 / 2;
            break;

        default:
            virtual_height = height;
            break;
    }

    pBo = CamOsMemCalloc(1, sizeof(struct bo));
    pBo->size = width * virtual_height * bpp / 8;
    pBo->ptr = (void *)((int)(pMemory));

    pBo->pitch = width * bpp / 8;
    pBo->offset = 0;

    virtual = pBo->ptr;

    /* just testing a limited # of formats to test single
     * and multi-planar path.. would be nice to add more..
     */
    switch(format)
    {
        case DRM_FORMAT_YUYV:
        case DRM_FORMAT_YVYU:
            offsets[0] = 0;
            pitches[0] = pBo->pitch;
            planes[0] = virtual;
            break;

        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV21:
            offsets[0] = 0;
            pitches[0] = pBo->pitch;
            pitches[1] = pitches[0];
            offsets[1] = pitches[0] * height;

            planes[0] = virtual;
            planes[1] = virtual + offsets[1];
            break;
    }

    UtilFillPattern(format, pattern, planes, width, height, pitches[0]);

    return pBo;
}

static void _DumpDigest(unsigned char *pDigest)
{
    int j;
    for(j = 0 ; j < 16; j++)
    {
        CamOsDebug("%02x", pDigest[j]);
    }
}


static int _CompareDigest(char *pDigestStrIn, unsigned char *pDigestHex)
{
    unsigned char j, tmp;
    unsigned char pDigestStr[33];

    for(j = 0 ; j < 16; j++)
    {
        //sprintf(pDigestStr + j*2, "%02x", pDigestHex[j]);
        tmp = (pDigestHex[j] & 0xF0) >> 4;
        pDigestStr[2 * j] = tmp >= 0x0a ? tmp - 10 + 'a' : tmp + '0';

        tmp = (pDigestHex[j] & 0x0F);
        pDigestStr[2 * j + 1] = tmp >= 0x0a ? tmp - 10 + 'a' : tmp + '0';
    }

    return memcmp(pDigestStrIn, pDigestStr, 32);
}

static void _PrepareInputBuffer_1(void)
{
    int nSize;
    int eRet = CAM_OS_OK;

    u32* pInUserPtr;
    u32  nInMiuAddr;
    u64  nlInPhysAddr;

    u32* pOutUserPtr;
    u32  nOutMiuAddr;
    u64  nlOutPhysAddr;

    // NOTE: 4177920 is used for 1920x1088 resolution, enlarge size if you want to test large resolution
    nSize = 4177920;

    eRet = CamOsDirectMemAlloc("JPE_IN1",
                        nSize,
                        (void**)&pInUserPtr,
                        &nInMiuAddr,
                        &nlInPhysAddr);
    if(CAM_OS_OK != eRet)
    {
        CamOsDebug("CamOsDirectMemAlloc fail\n");
    }

    eRet = CamOsDirectMemAlloc("JPE_OUT1",
                        nSize/2,
                        (void**)&pOutUserPtr,
                        &nOutMiuAddr,
                        &nlOutPhysAddr);
    if(CAM_OS_OK != eRet)
    {
        CamOsDebug("CamOsDirectMemAlloc fail\n");
    }

    _gKmaFileIn_1.user_ptr    = pInUserPtr;
    _gKmaFileIn_1.phys_addr   = (unsigned long)nlInPhysAddr;
    _gKmaFileIn_1.mem_size    = (unsigned long)nSize;
    _gKmaFileOut_1.user_ptr    = pOutUserPtr;
    _gKmaFileOut_1.phys_addr   = (unsigned long)nlOutPhysAddr;
    _gKmaFileOut_1.mem_size    = (unsigned long)nSize / 2;
}


static void _PrepareInputBuffer_2(void)
{
    int nSize;
    int eRet = CAM_OS_OK;

    u32* pInUserPtr;
    u32  nInMiuAddr;
    u64  nlInPhysAddr;

    u32* pOutUserPtr;
    u32  nOutMiuAddr;
    u64  nlOutPhysAddr;

    // NOTE: 4177920 is used for 1920x1088 resolution, enlarge size if you want to test large resolution
    nSize = 4177920;

    eRet = CamOsDirectMemAlloc("JPE_IN2",
                        nSize,
                        (void**)&pInUserPtr,
                        &nInMiuAddr,
                        &nlInPhysAddr);
    if(CAM_OS_OK != eRet)
    {
        CamOsDebug("CamOsDirectMemAlloc fail\n");
    }

    eRet = CamOsDirectMemAlloc("JPE_OUT2",
                        nSize/2,
                        (void**)&pOutUserPtr,
                        &nOutMiuAddr,
                        &nlOutPhysAddr);
    if(CAM_OS_OK != eRet)
    {
        CamOsDebug("CamOsDirectMemAlloc fail\n");
    }

    _gKmaFileIn_2.user_ptr    = pInUserPtr;
    _gKmaFileIn_2.phys_addr   = (unsigned long)nlInPhysAddr;
    _gKmaFileIn_2.mem_size    = (unsigned long)nSize;
    _gKmaFileOut_2.user_ptr    = pOutUserPtr;
    _gKmaFileOut_2.phys_addr   = (unsigned long)nlOutPhysAddr;
    _gKmaFileOut_2.mem_size    = (unsigned long)nSize / 2;
}

static void _ReleaseInputBuffer_1(void)
{
    CamOsDirectMemRelease(_gKmaFileIn_1.user_ptr, _gKmaFileIn_1.mem_size);
    CamOsDirectMemRelease(_gKmaFileOut_1.user_ptr, _gKmaFileOut_1.mem_size);
}

static void _ReleaseInputBuffer_2(void)
{
    CamOsDirectMemRelease(_gKmaFileIn_2.user_ptr, _gKmaFileIn_2.mem_size);
    CamOsDirectMemRelease(_gKmaFileOut_2.user_ptr, _gKmaFileOut_2.mem_size);
}

static int _DoEncode(JPEG_TEST_CLOCK_e eClkSelect, JPEG_TEST_RESOLUTION_e nRes, JPEG_TEST_FORMAT_e eRawFormat, int bDebugMsg, int nTaskId)
{
    int nDev;
    int  pFormat[] = {DRM_FORMAT_NV12, DRM_FORMAT_NV21, DRM_FORMAT_YUYV, DRM_FORMAT_YVYU};
    //char pSuffix[][5] = { "nv12", "nv21", "yuyv", "yvyu"};

    unsigned char pDigest[16];
    int nSize;
    int width = 320;
    int height = 240;

    unsigned int handles[4] = {0};
    unsigned int pitches[4] = {0};
    unsigned int offsets[4] = {0};
    UtilFillPattern_e ePattern = UTIL_PATTERN_SMPTE;

    md5_state_t tMd5State;
    struct bo *pBo;

    KMA_t *pKmaFileIn;    // used for YUV RAW data
    KMA_t *pKmaFileOut;    // used for JPEG encoded data

    if(_gTask1 == nTaskId)
    {
        pKmaFileIn = &_gKmaFileIn_1;
        pKmaFileOut = &_gKmaFileOut_1;
    }
    else
    {
        pKmaFileIn = &_gKmaFileIn_2;
        pKmaFileOut = &_gKmaFileOut_2;
    }

    switch(nRes)
    {
        case eRESOLUTION_CIF:
            width = 352;
            height = 288;
            break;

        case eRESOLUTION_VGA:
            width = 640;
            height = 480;
            break;

        case eRESOLUTION_1088P:
            width = 1920;
            height = 1088;
            break;

        case eRESOLUTION_1536P:
            width = 2688;
            height = 1536;
            break;

        default:
            CamOsDebug("unknow resolution\n");
            return JPEG_TEST_FAIL;
            break;
    }

    // Step 1. generate pattern
    pBo = _PatternCreate(pFormat[eRawFormat], width, height, handles, pitches, offsets, ePattern, pKmaFileIn->user_ptr);
    if(bDebugMsg)
    {
        CamOsDebug("Expected Pattern : size = %d, Hash = %s\n", _gpYUVHash[eRawFormat + 4 * nRes].size, _gpYUVHash[eRawFormat + 4 * nRes].pDigest);
        CamOsDebug("Actual   Pattern : size = %d, ", pBo->size);
    }

    if(_gpYUVHash[eRawFormat + 4 * nRes].size != pBo->size)
    {
        CamOsDebug("%sError!!!! Task(%d) Size(%d) mismatch with %d%s\n", RED, nTaskId, (int)(pBo->size), _gpYUVHash[eRawFormat + 4 * nRes].size, NORMAL);
    }

#if 0 // raw data hash is slow, so we only enable it when needed.
    memset(&tMd5State, sizeof(tMd5State), 0);
    Md5Init(&tMd5State);
    Md5Append(&tMd5State, pBo->ptr, pBo->size);
    Md5Finish(&tMd5State, pDigest);
    CamOsDebug("Hash = ");
    _DumpDigest(pDigest);
    CamOsDebug("\n\n");
#else
    CamOsDebug("\n");
#endif

    // Step 2. RAW data to JPEG (without header)
    JpeParam_t  tParam;
    JpeCfg_t    *pJpeCfg = &tParam.tJpeCfg;

    memset(pJpeCfg, 0, sizeof(JpeCfg_t));

    pJpeCfg->eInBufMode = JPE_IBUF_FRAME_MODE;
    pJpeCfg->eCodecFormat = JPE_CODEC_JPEG;

    if(eNV12 == eRawFormat)  pJpeCfg->eRawFormat = JPE_RAW_NV12;
    else if(eNV21 == eRawFormat)  pJpeCfg->eRawFormat = JPE_RAW_NV21;
    else if(eYUYV == eRawFormat)  pJpeCfg->eRawFormat = JPE_RAW_YUYV;
    else if(eYVYU == eRawFormat)  pJpeCfg->eRawFormat = JPE_RAW_YVYU;

    pJpeCfg->nWidth = width;
    pJpeCfg->nHeight = height;
    pJpeCfg->nQScale = JPEG_SCALE_FACTOR;
    memcpy(pJpeCfg->YQTable, std_luminance_quant_tbl, DCTSIZE2 * sizeof(unsigned short));
    memcpy(pJpeCfg->CQTable, std_chrominance_quant_tbl, DCTSIZE2 * sizeof(unsigned short));

    nSize = pBo->size;

    // Assign buffer for test
    pJpeCfg->OutBuf.nAddr = (unsigned long)pKmaFileOut->phys_addr;
    pJpeCfg->OutBuf.nSize = pKmaFileOut->mem_size;

    if(eYUYV == eRawFormat || eYVYU == eRawFormat)
    {
        pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr = (unsigned int)pKmaFileIn->phys_addr;
        pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nSize = nSize;
        pJpeCfg->InBuf[JPE_COLOR_PLAN_CHROMA].nAddr = 0;
        pJpeCfg->InBuf[JPE_COLOR_PLAN_CHROMA].nSize = 0;
    }
    else if(eNV21 == eRawFormat || eNV12 == eRawFormat)
    {
        pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr = (unsigned int)pKmaFileIn->phys_addr;
        pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nSize = width * height;
        pJpeCfg->InBuf[JPE_COLOR_PLAN_CHROMA].nAddr = (unsigned int)(pKmaFileIn->phys_addr + width * height);
        pJpeCfg->InBuf[JPE_COLOR_PLAN_CHROMA].nSize = nSize - width * height;
    }

    tParam.nClkSelect = (unsigned short)eClkSelect;

    nDev = open(DEVICE_PATH, O_RDWR);
    if(nDev <= 0)
    {
        CamOsDebug("Can't open device %s return=%d errno=%d\n", DEVICE_PATH, nDev, errno);
        return -1;
    }
    JpeEncode(&tParam, nDev);

    // Step 3. generate hash for JPEG encoded data
    memset(&tMd5State, sizeof(tMd5State), 0);
    Md5Init(&tMd5State);
    // omit the end of JFIF (0xFFD9)
    //Md5Append(&tMd5State, (const Md5Byte_t *)pJpeCfg->OutBuf.nAddr, (int)tParam.nEncodeSize - 2);
    Md5Append(&tMd5State, (const Md5Byte_t *)pKmaFileOut->user_ptr, (int)tParam.nEncodeSize - 2);
    Md5Finish(&tMd5State, pDigest);

    if(tParam.nEncodeSize > pJpeCfg->OutBuf.nSize)
    {
        CamOsDebug("%sTask(%d) Error!!!! nEncodeSize(%d) > OutBufSize(%d). This should never happen!! \n", RED, nTaskId, (int)(tParam.nEncodeSize), pJpeCfg->OutBuf.nSize, NORMAL);
    }

    if(_gpJpegBinHash[eRawFormat + 4 * nRes].size != tParam.nEncodeSize - 2)
    {
        CamOsDebug("%sTask(%d) Error!!!! Size(%d) mismatch with %d%s\n", RED, nTaskId, (int)(tParam.nEncodeSize - 2), _gpJpegBinHash[eRawFormat + 4 * nRes].size, NORMAL);
    }

    if(0 != _CompareDigest(_gpJpegBinHash[eRawFormat + 4 * nRes].pDigest, pDigest))
    {
        CamOsDebug("%sTask(%d) Error!!!!  Digest(", RED, nTaskId);
        _DumpDigest(pDigest);
        CamOsDebug(") mismatch with %s%s\n", _gpJpegBinHash[eRawFormat + 4 * nRes].pDigest, NORMAL);
    }
    else
    {
        if(bDebugMsg)
        {
            CamOsDebug("Expected result size is %d\n", _gpJpegBinHash[eRawFormat + 4 * nRes].size);
            CamOsDebug("Expected Hash is %s\n", _gpJpegBinHash[eRawFormat + 4 * nRes].pDigest);
            CamOsDebug("Actual   Hash is ");
            _DumpDigest(pDigest);
            CamOsDebug("\n");
        }
        CamOsDebug("Encode done. Hash value for %dx%d check pass.\n\n", width, height);

    }

    _PatternRelease(pBo);

    return JPEG_TEST_SUCCESS;
}

static int _DoTest(JPEG_TEST_FORMAT_e eRawFormat, int bDebugMsg, int nTaskId)
{
    int nRet;
    JPEG_TEST_CLOCK_e eClkSelect = eCLK_288;
    JPEG_TEST_RESOLUTION_e eRes = eRESOLUTION_1088P;

    CamOsDebug("====");
    switch(eRawFormat)
    {
        case eNV12:
            CamOsDebug(" Raw format: eNV12 ");
            break;
        case eNV21:
            CamOsDebug(" Raw format: eNV21 ");
            break;
        case eYUYV:
            CamOsDebug(" Raw format: eYUYV ");
            break;
        case eYVYU:
            CamOsDebug(" Raw format: eYVYU ");
            break;
    }
    CamOsDebug(" ====\n");


#if JPE_DRIVER_VER == 2
    for(eClkSelect = eCLK_288; eClkSelect <= eCLK_27; eClkSelect++)
#else
    // do nothing,  JPE Driver V1 don't support clock rate change.
#endif
    {

        switch(eClkSelect)
        {
            case eCLK_288:
                CamOsDebug("%s Task:0x%x Clock: eCLK_288 %s \n", BLUE, nTaskId, NORMAL);
                break;
            case eCLK_216:
                CamOsDebug("%s Task:0x%x Clock: eCLK_216 %s \n", BLUE, nTaskId, NORMAL);
                break;
            case eCLK_54:
                CamOsDebug("%s Task:0x%x Clock: eCLK_54  %s \n", BLUE, nTaskId, NORMAL);
                break;
            case eCLK_27:
                CamOsDebug("%s Task:0x%x Clock: eCLK_27  %s \n", BLUE, nTaskId, NORMAL);
                break;
        }

        for(eRes = eRESOLUTION_CIF; eRes <= eRESOLUTION_1088P; eRes++)
        {
            switch(eRes)
            {
                case eRESOLUTION_CIF:
                    CamOsDebug("%s Res: CIF %s \n", BLUE, NORMAL);
                    break;
                case eRESOLUTION_VGA:
                    CamOsDebug("%s Res: VGA %s \n", BLUE, NORMAL);
                    break;
                case eRESOLUTION_1088P:
                    CamOsDebug("%s Res: 1920x1088 %s \n", BLUE, NORMAL);
                    break;
                case eRESOLUTION_1536P:
                    CamOsDebug("%s Res: 2688x1536 %s \n", BLUE, NORMAL);
                    break;
            }

            nRet = _DoEncode(eClkSelect, eRes, eRawFormat, bDebugMsg, nTaskId);
            if(JPEG_TEST_SUCCESS != nRet)
            {
                CamOsDebug("%s Error!!!! doEncode for resolution (%d) error!! %s\n", RED, eRes, NORMAL);
                return -1;
            }
        }
    }
    return 0;

}

static void * _TestJpeTask1(void *pArg)
{
    int i;
    _PrepareInputBuffer_1();
    for(i = 0; i < JPEG_TEST_COUNT; i++)
    {
        _DoTest(eYUYV, _gDbgMsgFlag, _gTask1);
    }
    _ReleaseInputBuffer_1();
    return NULL;
}

static void * _TestJpeTask2(void *pArg)
{
    int i;
    _PrepareInputBuffer_2();
    for(i = 0; i < JPEG_TEST_COUNT; i++)
    {
        _DoTest(eNV12, _gDbgMsgFlag, _gTask2);
    }
    _ReleaseInputBuffer_2();
    return NULL;
}

int TestJpeDriverWith2Tasks()
{
    // enable or disable debug message
    _gDbgMsgFlag = 0;

    CamOsDebug("%s start of %s %s\n", BLUE, __func__, NORMAL);

    // Create Task here
    pthread_create(&_gTask1, NULL, _TestJpeTask1, NULL);
    pthread_create(&_gTask2, NULL, _TestJpeTask2, NULL);

    pthread_join(_gTask1, NULL);
    CamOsDebug("Joined with thread1 0x%x\n",_gTask1);

    pthread_join(_gTask2, NULL);
    CamOsDebug("Joined with thread2 0x%x\n",_gTask2);

    CamOsDebug("%s End of %s %s\n", BLUE, __func__, NORMAL);

    return 0;
}