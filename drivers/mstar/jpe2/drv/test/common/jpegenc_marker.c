#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "jpegenc_marker.h"

#define JPG_BOOL_TRUE     1
#define JPG_BOOL_FALSE    0
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                      Internal structure for JPEG headers                                                         //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct omx_jpegenc_marker
{
    struct list_head node;
    u16            size;
    void             *data;
} omx_jpegenc_marker;

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

typedef struct omx_jpegenc_marker_soi
{
    u16 marker;                   // Start of image marker (FFD8)
} omx_jpegenc_marker_soi;

typedef struct omx_jpegenc_marker_app0_jfif
{
    u16 marker;                   // App0 marker (FFE0)
    u16 length;                   // Header length
    u8  identifier[5];            // "JFIF" string
    u8  version[2];               // JFIF version
    u8  density_unit;             // 0 = no unit, 1 = pixels per inch, 2 = pixels per centimeter
    u16 density_x;                // Horizontal pixel density. Must not be zero.
    u16 density_y;                // Vertical pixel density. Must not be zero.
    u8  thumbnail_wdith;          // Horizontal pixel count of the following embedded RGB thumbnail. May be zero
    u8  thumbnail_height;         // Vertical pixel count of the following embedded RGB thumbnail. May be zero
} omx_jpegenc_marker_app0_jfif;

typedef struct omx_jpegenc_marker_dqt_8
{
    u16 marker;                   // Define quantization table marker (FFDB)
    u16 length;                   // Hheader length
    u8  index:4;                  // Quantization table destination identifier
    u8  precision:4;              // Quantization table element precision. 0 = 8bit, 1 = 16bit
    u8  dqt_data[64];             // Quantization table entries, 8 bit width
} omx_jpegenc_marker_dqt_8;

typedef struct omx_jpegenc_marker_dqt_16
{
    u16 marker;                   // Define quantization table marker (FFDB)
    u16 length;                   // Hheader length
    u8  index:4;                  // Quantization table destination identifier
    u8  precision:4;              // Quantization table element precision. 0: 8bit, 1: 16bit
    u16 dqt_data[64];             // Quantization table entries, 16 bit width
} omx_jpegenc_marker_dqt_16;

typedef struct omx_jpegenc_marker_sof0_yuv
{
    u16 marker;                   // Baseline DCT marker (FFC0)
    u16 length;                   // Hheader length
    u8  sample_precision;         // Sample precision
    u16 height;                   // Number of lines
    u16 width;                    // Number of samples per line
    u8  num_components;           // Number of image components in frame, yuv is 3
    struct
    {                        // Conpoment info
        u8  index;                // Component identifier
        u8  sampling_x:4;         // Horizontal sampling factor
        u8  sampling_y:4;         // Vertical sampling factor
        u8  dqt_idx;              // Quantization table destination selector
    } components[3];
} omx_jpegenc_marker_sof0_yuv;

typedef struct omx_jpegenc_marker_dht_baseline_dc
{
    u16 marker;                   // Define Huffman table marker (FFC4)
    u16 length;                   // Header length
    u8  index:4;                  // Huffman table destination identifier
    u8  class:4;                  // Table class – 0 = DC table or lossless table, 1 = AC table.
    u8  code_length[16];          // Number of Huffman codes of length
    u8  code_table[12];           // Value associated with each Huffman code
} omx_jpegenc_marker_dht_baseline_dc;

typedef struct omx_jpegenc_marker_dht_baseline_ac
{
    u16 marker;                   // Define Huffman table marker (FFC4)
    u16 length;                   // Header length
    u8  index:4;                  // Huffman table destination identifier
    u8  class:4;                  // Table class – 0 = DC table or lossless table, 1 = AC table.
    u8  code_length[16];          // Number of Huffman codes of length
    u8  code_table[162];          // Value associated with each Huffman code
} omx_jpegenc_marker_dht_baseline_ac;

typedef struct omx_jpegenc_marker_sos_baseline
{
    u16 marker;                   // Start of scan marker (FFDA)
    u16 length;                   // Header length
    u8  num_components;           // Number of image components in scan
    struct
    {
        u8  component_select;     // Scan component selector
        u8  ac_select:4;          // AC entropy coding table destination selector
        u8  dc_select:4;          // DC entropy coding table destination selector
    } select[3];
    u8  start_select;             // Start of spectral or predictor selection
    u8  end_select;               // End of spectral selection
    u8  al:4;                     // Successive approximation bit position high
    u8  ah:4;                     // Successive approximation bit position low or point transform
} omx_jpegenc_marker_sos_baseline;

#pragma pack(pop)  /* push current alignment to stack */

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                    Macros                                                                        //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if BYTE_ORDER == BIG_ENDIAN
#define BIG_E_ASSIGN_U16(v) (v)

#elif BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == PDP_ENDIAN
static inline u16 _bit_e_assign_u16(u16 v) { return ((v<<8)&0xFF00) | ((v>>8)&0x00FF); }
#define BIG_E_ASSIGN_U16(v) ((((v)<<8)&0xFF00) | (((v)>>8)&0x00FF))// _bit_e_assign_u16((u16)(v))

#else
#error Unknow endian type....
#endif

#define GET_MARKER(m) (((u16*)((m)->data))[0])
#define GET_LENGTH(m) (((u16*)((m)->data))[1])

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                             internal functions                                                                   //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static omx_jpegenc_marker* omx_jpegenc_marker_alloc(size_t size, const void* data)
{
    omx_jpegenc_marker *marker;

    // Allocate buffer
    marker = malloc(sizeof(omx_jpegenc_marker) + size);
    if(marker == NULL)
        return NULL;
    //JPE_DEBUG(DEB_LEV_PARAMS, "%s()line-%d malloc: %p, size=%u\n", __func__, __LINE__, marker, sizeof(omx_jpegenc_marker) + size);

    // Assign marker data pointer, force the pointer after marker
    marker->data = (void*)(&marker[1]);;
    marker->size = size;

    // Copy data
    if(data != NULL)
    {
        memcpy(marker->data, data, size);
    }

    return marker;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                      internal baseline functions                                                                 //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool omx_jpegenc_marker_baseline_add_soi(struct list_head *head)
{
    omx_jpegenc_marker *marker = omx_jpegenc_marker_alloc(sizeof(omx_jpegenc_marker_soi), NULL);
    omx_jpegenc_marker_soi *soi;

    if(marker == NULL) return JPG_BOOL_FALSE;
    soi = (omx_jpegenc_marker_soi*)marker->data;

    // Init SOI data
    soi->marker = BIG_E_ASSIGN_U16(JPEG_MARKER_SOI);

    // Add to list
    list_add_tail(&marker->node, head);

    return JPG_BOOL_TRUE;
}

static const omx_jpegenc_marker_app0_jfif  __omx_jpegenc_marker_app0_jfif = {
    .marker             = BIG_E_ASSIGN_U16(JPEG_MARKER_APP0),
    .length             = BIG_E_ASSIGN_U16(sizeof(omx_jpegenc_marker_app0_jfif)-2),
    .identifier         = "JFIF",
    .version            = {1, 1},
    .density_unit       = 2,
    .density_x          = BIG_E_ASSIGN_U16(118),
    .density_y          = BIG_E_ASSIGN_U16(118),
    .thumbnail_wdith    = 0,
    .thumbnail_height   = 0
};

static bool omx_jpegenc_marker_baseline_add_app0(struct list_head *head)
{
    omx_jpegenc_marker *marker = omx_jpegenc_marker_alloc(sizeof(omx_jpegenc_marker_app0_jfif), &__omx_jpegenc_marker_app0_jfif);

    if(marker == NULL)
        return JPG_BOOL_FALSE;

    list_add_tail(&marker->node, head);

    return JPG_BOOL_TRUE;
}

static const u8 zig_zag_idx[64] =
{
     0,
     1,  8,
    16,  9,  2,
     3, 10, 17, 24,
    32, 25, 18, 11,  4,
     5, 12, 19, 26, 33, 40,
    48, 41, 34, 27, 20, 13,  6,
     7, 14, 21, 28, 35, 42, 49, 56,
    57, 50, 43, 36, 29, 22, 15,
    23, 30, 37, 44, 51, 58,
    59, 52, 45, 38, 31,
    39, 46, 53, 60,
    61, 54, 47,
    55, 62,
    63
};
static bool omx_jpegenc_marker_baseline_add_dqt(struct list_head *head, u16 *y_table, u16 *c_table, u16 scale)
{
    omx_jpegenc_marker *marker;
    omx_jpegenc_marker_dqt_8 *dqt;
    u8 i, j;
    u16 *table = y_table;

    for(i=0; i<2; i++, table=c_table)
    {
        // Assign marker data pointer, force the pointer after marker
        marker = omx_jpegenc_marker_alloc(sizeof(omx_jpegenc_marker_dqt_8), NULL);
        if(marker == NULL) return JPG_BOOL_FALSE;
        dqt = (omx_jpegenc_marker_dqt_8*)marker->data;

        // Init DQT
        dqt->marker    = BIG_E_ASSIGN_U16(JPEG_MARKER_DQT);
        dqt->length    = BIG_E_ASSIGN_U16(sizeof(omx_jpegenc_marker_dqt_8)-2);
        dqt->index     = i;
        dqt->precision = 0; // 8 bit

        for(j=0; j<64; j++)
        {
          dqt->dqt_data[j] = (u8)(((unsigned long)table[zig_zag_idx[j]]*(unsigned long)scale+50)/100);
          dqt->dqt_data[j] = dqt->dqt_data[j] ? dqt->dqt_data[j] : 1;
        }

        // Add to list
        list_add_tail(&marker->node, head);
    }

    return JPG_BOOL_TRUE;
}

static const omx_jpegenc_marker_sof0_yuv __omx_jpegenc_marker_sof0_yuv =
{
    .marker             = BIG_E_ASSIGN_U16(JPEG_MARKER_SOF0),
    .length             = BIG_E_ASSIGN_U16(sizeof(omx_jpegenc_marker_sof0_yuv)-2),
    .sample_precision   = 8,
    .width              = 0,
    .height             = 0,
    .num_components     = 3,
    .components         = { {1,1,2,0}, {2,1,1,1}, {3,1,1,1}}
};

static bool omx_jpegenc_marker_baseline_add_sof(struct list_head *head, bool yuv422, u16 width, u16 height)
{
    omx_jpegenc_marker *marker = omx_jpegenc_marker_alloc(sizeof(omx_jpegenc_marker_sof0_yuv), &__omx_jpegenc_marker_sof0_yuv);
    omx_jpegenc_marker_sof0_yuv *sof;

    if(marker == NULL)
        return JPG_BOOL_FALSE;

    sof = (omx_jpegenc_marker_sof0_yuv*)marker->data;

    // Init SOf data
    sof->height = BIG_E_ASSIGN_U16(height);
    sof->width  = BIG_E_ASSIGN_U16(width);

    if(yuv422)
    {
        sof->components[0].sampling_x = 1;
    }
    else
    {
        sof->components[0].sampling_x = 2;
    }

    // Add to list
    list_add_tail(&marker->node, head);

    return JPG_BOOL_TRUE;
}

static const omx_jpegenc_marker_dht_baseline_dc __omx_jpegenc_marker_dht_baseline_dc_y =
{
    .marker         = BIG_E_ASSIGN_U16(JPEG_MARKER_DHT),
    .length         = BIG_E_ASSIGN_U16(sizeof(omx_jpegenc_marker_dht_baseline_dc)-2),
    .index          = 0,
    .class          = 0,
    .code_length    = {0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .code_table     = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B}
};

static const omx_jpegenc_marker_dht_baseline_ac __omx_jpegenc_marker_dht_baseline_ac_y =
{
    .marker         = BIG_E_ASSIGN_U16(JPEG_MARKER_DHT),
    .length         = BIG_E_ASSIGN_U16(sizeof(omx_jpegenc_marker_dht_baseline_ac)-2),
    .index          = 0,
    .class          = 1,
    .code_length    = {0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D},
    .code_table     = {0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
                       0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
                       0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
                       0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
                       0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
                       0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
                       0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
                       0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
                       0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
                       0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
                       0xF9, 0xFA}
};

static const omx_jpegenc_marker_dht_baseline_dc __omx_jpegenc_marker_dht_baseline_dc_c =
{
    .marker         = BIG_E_ASSIGN_U16(JPEG_MARKER_DHT),
    .length         = BIG_E_ASSIGN_U16(sizeof(omx_jpegenc_marker_dht_baseline_dc)-2),
    .index          = 1,
    .class          = 0,
    .code_length    = {0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
    .code_table     = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B}
};

static const omx_jpegenc_marker_dht_baseline_ac __omx_jpegenc_marker_dht_baseline_ac_c =
{
    .marker         = BIG_E_ASSIGN_U16(JPEG_MARKER_DHT),
    .length         = BIG_E_ASSIGN_U16(sizeof(omx_jpegenc_marker_dht_baseline_ac)-2),
    .index          = 1,
    .class          = 1,
    .code_length    = {0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77},
    .code_table     = {0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
                       0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
                       0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
                       0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                       0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                       0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
                       0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
                       0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
                       0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
                       0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
                       0xF9, 0xFA}
};

static bool omx_jpegenc_marker_baseline_add_dht(struct list_head *head)
{
    omx_jpegenc_marker *marker;

    // Allocate buffer for DC DHT of Y
    marker = omx_jpegenc_marker_alloc(sizeof(omx_jpegenc_marker_dht_baseline_dc), &__omx_jpegenc_marker_dht_baseline_dc_y);
    if(marker == NULL) return JPG_BOOL_FALSE;
    list_add_tail(&marker->node, head);

    // Allocate buffer for AC DHT of Y
    marker = omx_jpegenc_marker_alloc(sizeof(omx_jpegenc_marker_dht_baseline_ac), &__omx_jpegenc_marker_dht_baseline_ac_y);
    if(marker == NULL) return JPG_BOOL_FALSE;
    list_add_tail(&marker->node, head);

    // Allocate buffer for DC DHT of C
    marker = omx_jpegenc_marker_alloc(sizeof(omx_jpegenc_marker_dht_baseline_dc), &__omx_jpegenc_marker_dht_baseline_dc_c);
    if(marker == NULL) return JPG_BOOL_FALSE;
    list_add_tail(&marker->node, head);

    // Allocate buffer for AC DHT of C
    marker = omx_jpegenc_marker_alloc(sizeof(omx_jpegenc_marker_dht_baseline_ac), &__omx_jpegenc_marker_dht_baseline_ac_c);
    if(marker == NULL) return JPG_BOOL_FALSE;
    list_add_tail(&marker->node, head);

    return JPG_BOOL_TRUE;
}

static const omx_jpegenc_marker_sos_baseline __omx_jpegenc_marker_sos_baseline =
{
    .marker         = BIG_E_ASSIGN_U16(JPEG_MARKER_SOS),
    .length         = BIG_E_ASSIGN_U16(sizeof(omx_jpegenc_marker_sos_baseline)-2),
    .num_components = 3,
    .select         = {{1,0,0}, {2,1,1}, {3,1,1}},
    .start_select   = 0,
    .end_select     = 0x3F,
    .al             = 0,
    .ah             = 0
};

static bool omx_jpegenc_marker_baseline_add_sos(struct list_head *head)
{
    omx_jpegenc_marker *marker = omx_jpegenc_marker_alloc(sizeof(omx_jpegenc_marker_sos_baseline), &__omx_jpegenc_marker_sos_baseline);;

    if(marker == NULL)
        return JPG_BOOL_FALSE;

    list_add_tail(&marker->node, head);

    return JPG_BOOL_TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                             External functions                                                                   //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void omx_jpegenc_marker_init(struct list_head *head)
{
    INIT_LIST_HEAD(head);
}

void omx_jpegenc_marker_release(struct list_head *head)
{
    struct omx_jpegenc_marker *marker, *temp;

    // Release all buffers
    list_for_each_entry_safe(marker, temp, head, node)
    {
        list_del(&marker->node);
        free(marker);
        //JPE_DEBUG(DEB_LEV_PARAMS, "%s()line-%d free: %p\n", __func__, __LINE__, marker);
    }
}

void* omx_jpegenc_marker_create_app_n(struct list_head *head, JPEG_MARKER_TYPE type, u16 size)
{
    struct omx_jpegenc_marker *mk;
    omx_jpegenc_marker *marker = omx_jpegenc_marker_alloc(size+4, NULL);

    if(marker == NULL)
        return NULL;

    ((u16*)(marker->data))[0] = BIG_E_ASSIGN_U16(type);
    ((u16*)(marker->data))[1] = BIG_E_ASSIGN_U16(size+2); // The size should be the total length - marker length


    list_for_each_entry(mk, head, node)
    {
        if( GET_MARKER(mk) == BIG_E_ASSIGN_U16(JPEG_MARKER_APP0))
        {
            list_add(&marker->node, &mk->node);
            break;
        }
    }

    return marker->data + 4;
}

int omx_jpegenc_marker_size(struct list_head *head)
{
    omx_jpegenc_marker *marker;
    int size = 0;

    list_for_each_entry(marker, head, node)
    {
        size += marker->size;
    }

    return size;
}

int omx_jpegenc_marker_dump(struct list_head *head, u8 *data)
{
    omx_jpegenc_marker *marker;
    int size = 0;

    list_for_each_entry(marker, head, node)
    {
        memcpy(data + size, marker->data, marker->size);
        size += marker->size;
    }

    return size;
}

bool omx_jpegenc_marker_create_baseline(struct list_head *head, bool yuv422, u16 width, u16 height, u16 *dqt_table_y, u16 *dqt_table_c, u16 dqt_scale)
{
    // SOS (FFD8)
    if(!omx_jpegenc_marker_baseline_add_soi(head)) return JPG_BOOL_FALSE;

    // APP0 (FFE0)
    if(!omx_jpegenc_marker_baseline_add_app0(head)) return JPG_BOOL_FALSE;

    // DQT (FFEB) x 2
    if(!omx_jpegenc_marker_baseline_add_dqt(head, dqt_table_y, dqt_table_c, dqt_scale)) return JPG_BOOL_FALSE;

    // SOF0 (FFC0)
    if(!omx_jpegenc_marker_baseline_add_sof(head, yuv422, width, height)) return JPG_BOOL_FALSE;

    // DHT (FFC4) x 4
    if(!omx_jpegenc_marker_baseline_add_dht(head)) return JPG_BOOL_FALSE;

    // SOS (FFDA)
    if(!omx_jpegenc_marker_baseline_add_sos(head)) return JPG_BOOL_FALSE;

    return JPG_BOOL_TRUE;
}