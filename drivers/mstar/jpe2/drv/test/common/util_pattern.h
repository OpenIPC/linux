#ifndef __UTIL_PATTERN_H__
#define __UTIL_PATTERN_H__

#ifndef uint32_t
#define uint32_t unsigned int
#endif

#define FOURCC_CODE(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
                                 ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define DRM_FORMAT_YUYV		FOURCC_CODE('Y', 'U', 'Y', 'V') /* [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian */
#define DRM_FORMAT_YVYU		FOURCC_CODE('Y', 'V', 'Y', 'U') /* [31:0] Cb0:Y1:Cr0:Y0 8:8:8:8 little endian */
#define DRM_FORMAT_NV12		FOURCC_CODE('N', 'V', '1', '2') /* 2x2 subsampled Cr:Cb plane */
#define DRM_FORMAT_NV21		FOURCC_CODE('N', 'V', '2', '1') /* 2x2 subsampled Cb:Cr plane */


typedef struct UtilColorComponent
{
    unsigned int length;
    unsigned int offset;
} UtilColorComponent_t;


typedef enum UtilYuvOrder
{
    YUV_YCbCr = 1,
    YUV_YCrCb = 2,
    YUV_YC = 4,
    YUV_CY = 8,
} UtilYuvOrder_e;


typedef struct UtilYuvInfo
{
    UtilYuvOrder_e order;
    unsigned int xsub;
    unsigned int ysub;
    unsigned int chroma_stride;
} UtilYuvInfo_t;


typedef struct UtilFormatInfo
{
    uint32_t format;
    const char *name;
    const UtilYuvInfo_t yuv;
} UtilFormatInfo_t;

typedef enum UtilFillPattern
{
    UTIL_PATTERN_TILES,
    UTIL_PATTERN_PLAIN,
    UTIL_PATTERN_SMPTE,
} UtilFillPattern_e;


void UtilFillPattern(uint32_t format,
                     UtilFillPattern_e pattern,
                     void *planes[3],
                     unsigned int width,
                     unsigned int height,
                     unsigned int stride);
#endif /* UTIL_PATTERN_H */