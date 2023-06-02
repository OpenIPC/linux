/*
 * Copyright 2008 Tungsten Graphics
 *   Jakob Bornecrantz <jakob@tungstengraphics.com>
 * Copyright 2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util_pattern.h"

// From format.c
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define MAKE_YUV_INFO(order, xsub, ysub, chroma_stride) \
    .yuv = { (order), (xsub), (ysub), (chroma_stride) }

static const UtilFormatInfo_t format_info[] =
{
    /* YUV packed */
    { DRM_FORMAT_YUYV, "YUYV", MAKE_YUV_INFO(YUV_YCbCr | YUV_YC, 2, 2, 2) },
    { DRM_FORMAT_YVYU, "YVYU", MAKE_YUV_INFO(YUV_YCrCb | YUV_YC, 2, 2, 2) },
    /* YUV semi-planar */
    { DRM_FORMAT_NV12, "NV12", MAKE_YUV_INFO(YUV_YCbCr, 2, 2, 2) },
    { DRM_FORMAT_NV21, "NV21", MAKE_YUV_INFO(YUV_YCrCb, 2, 2, 2) },
};

uint32_t util_format_fourcc(const char *name)
{
    unsigned int i;

    for(i = 0; i < ARRAY_SIZE(format_info); i++)
        if(!strcmp(format_info[i].name, name))
            return format_info[i].format;

    return 0;
}

const UtilFormatInfo_t *util_format_info_find(uint32_t format)
{
    unsigned int i;

    for(i = 0; i < ARRAY_SIZE(format_info); i++)
        if(format_info[i].format == format)
            return &format_info[i];

    return NULL;
}

// From pattern.c
struct color_yuv
{
    unsigned char y;
    unsigned char u;
    unsigned char v;
};

#define MAKE_YUV_601_Y(r, g, b) \
    ((( 66 * (r) + 129 * (g) +  25 * (b) + 128) >> 8) + 16)
#define MAKE_YUV_601_U(r, g, b) \
    (((-38 * (r) -  74 * (g) + 112 * (b) + 128) >> 8) + 128)
#define MAKE_YUV_601_V(r, g, b) \
    (((112 * (r) -  94 * (g) -  18 * (b) + 128) >> 8) + 128)

#define MAKE_YUV_601(r, g, b) \
    { .y = MAKE_YUV_601_Y(r, g, b), \
           .u = MAKE_YUV_601_U(r, g, b), \
                .v = MAKE_YUV_601_V(r, g, b) }

#define MAKE_RGBA(rgb, r, g, b, a) \
    ((((r) >> (8 - (rgb)->red.length)) << (rgb)->red.offset) | \
     (((g) >> (8 - (rgb)->green.length)) << (rgb)->green.offset) | \
     (((b) >> (8 - (rgb)->blue.length)) << (rgb)->blue.offset) | \
     (((a) >> (8 - (rgb)->alpha.length)) << (rgb)->alpha.offset))

#define MAKE_RGB24(rgb, r, g, b) \
    { .value = MAKE_RGBA(rgb, r, g, b, 0) }

static void fill_smpte_yuv_planar(const UtilYuvInfo_t *yuv,
                                  unsigned char *y_mem, unsigned char *u_mem,
                                  unsigned char *v_mem, unsigned int width,
                                  unsigned int height, unsigned int stride)
{
    const struct color_yuv colors_top[] =
    {
        MAKE_YUV_601(191, 192, 192),	/* grey */
        MAKE_YUV_601(192, 192, 0),	/* yellow */
        MAKE_YUV_601(0, 192, 192),	/* cyan */
        MAKE_YUV_601(0, 192, 0),	/* green */
        MAKE_YUV_601(192, 0, 192),	/* magenta */
        MAKE_YUV_601(192, 0, 0),	/* red */
        MAKE_YUV_601(0, 0, 192),	/* blue */
    };

    const struct color_yuv colors_middle[] =
    {
        MAKE_YUV_601(0, 0, 192),	/* blue */
        MAKE_YUV_601(19, 19, 19),	/* black */
        MAKE_YUV_601(192, 0, 192),	/* magenta */
        MAKE_YUV_601(19, 19, 19),	/* black */
        MAKE_YUV_601(0, 192, 192),	/* cyan */
        MAKE_YUV_601(19, 19, 19),	/* black */
        MAKE_YUV_601(192, 192, 192),	/* grey */
    };

    const struct color_yuv colors_bottom[] =
    {
        MAKE_YUV_601(0, 33, 76),	/* in-phase */
        MAKE_YUV_601(255, 255, 255),	/* super white */
        MAKE_YUV_601(50, 0, 106),	/* quadrature */
        MAKE_YUV_601(19, 19, 19),	/* black */
        MAKE_YUV_601(9, 9, 9),		/* 3.5% */
        MAKE_YUV_601(19, 19, 19),	/* 7.5% */
        MAKE_YUV_601(29, 29, 29),	/* 11.5% */
        MAKE_YUV_601(19, 19, 19),	/* black */
    };

    unsigned int cs = yuv->chroma_stride;
    unsigned int xsub = yuv->xsub;
    unsigned int ysub = yuv->ysub;
    unsigned int x;
    unsigned int y;

    /* Luma */
    for(y = 0; y < height * 6 / 9; ++y)
    {
        for(x = 0; x < width; ++x)
            y_mem[x] = colors_top[x * 7 / width].y;
        y_mem += stride;
    }

    for(; y < height * 7 / 9; ++y)
    {
        for(x = 0; x < width; ++x)
            y_mem[x] = colors_middle[x * 7 / width].y;
        y_mem += stride;
    }

    for(; y < height; ++y)
    {
        for(x = 0; x < width * 5 / 7; ++x)
            y_mem[x] = colors_bottom[x * 4 / (width * 5 / 7)].y;
        for(; x < width * 6 / 7; ++x)
            y_mem[x] = colors_bottom[(x - width * 5 / 7) * 3
                                     / (width / 7) + 4].y;
        for(; x < width; ++x)
            y_mem[x] = colors_bottom[7].y;
        y_mem += stride;
    }

    /* Chroma */
    for(y = 0; y < height / ysub * 6 / 9; ++y)
    {
        for(x = 0; x < width; x += xsub)
        {
            u_mem[x * cs / xsub] = colors_top[x * 7 / width].u;
            v_mem[x * cs / xsub] = colors_top[x * 7 / width].v;
        }
        u_mem += stride * cs / xsub;
        v_mem += stride * cs / xsub;
    }

    for(; y < height / ysub * 7 / 9; ++y)
    {
        for(x = 0; x < width; x += xsub)
        {
            u_mem[x * cs / xsub] = colors_middle[x * 7 / width].u;
            v_mem[x * cs / xsub] = colors_middle[x * 7 / width].v;
        }
        u_mem += stride * cs / xsub;
        v_mem += stride * cs / xsub;
    }

    for(; y < height / ysub; ++y)
    {
        for(x = 0; x < width * 5 / 7; x += xsub)
        {
            u_mem[x * cs / xsub] =
                colors_bottom[x * 4 / (width * 5 / 7)].u;
            v_mem[x * cs / xsub] =
                colors_bottom[x * 4 / (width * 5 / 7)].v;
        }
        for(; x < width * 6 / 7; x += xsub)
        {
            u_mem[x * cs / xsub] = colors_bottom[(x - width * 5 / 7) *
                                                 3 / (width / 7) + 4].u;
            v_mem[x * cs / xsub] = colors_bottom[(x - width * 5 / 7) *
                                                 3 / (width / 7) + 4].v;
        }
        for(; x < width; x += xsub)
        {
            u_mem[x * cs / xsub] = colors_bottom[7].u;
            v_mem[x * cs / xsub] = colors_bottom[7].v;
        }
        u_mem += stride * cs / xsub;
        v_mem += stride * cs / xsub;
    }
}

static void fill_smpte_yuv_packed(const UtilYuvInfo_t *yuv, void *mem,
                                  unsigned int width, unsigned int height,
                                  unsigned int stride)
{
    const struct color_yuv colors_top[] =
    {
        MAKE_YUV_601(191, 192, 192),	/* grey */
        MAKE_YUV_601(192, 192, 0),	/* yellow */
        MAKE_YUV_601(0, 192, 192),	/* cyan */
        MAKE_YUV_601(0, 192, 0),	/* green */
        MAKE_YUV_601(192, 0, 192),	/* magenta */
        MAKE_YUV_601(192, 0, 0),	/* red */
        MAKE_YUV_601(0, 0, 192),	/* blue */
    };

    const struct color_yuv colors_middle[] =
    {
        MAKE_YUV_601(0, 0, 192),	/* blue */
        MAKE_YUV_601(19, 19, 19),	/* black */
        MAKE_YUV_601(192, 0, 192),	/* magenta */
        MAKE_YUV_601(19, 19, 19),	/* black */
        MAKE_YUV_601(0, 192, 192),	/* cyan */
        MAKE_YUV_601(19, 19, 19),	/* black */
        MAKE_YUV_601(192, 192, 192),	/* grey */
    };

    const struct color_yuv colors_bottom[] =
    {
        MAKE_YUV_601(0, 33, 76),	/* in-phase */
        MAKE_YUV_601(255, 255, 255),	/* super white */
        MAKE_YUV_601(50, 0, 106),	/* quadrature */
        MAKE_YUV_601(19, 19, 19),	/* black */
        MAKE_YUV_601(9, 9, 9),		/* 3.5% */
        MAKE_YUV_601(19, 19, 19),	/* 7.5% */
        MAKE_YUV_601(29, 29, 29),	/* 11.5% */
        MAKE_YUV_601(19, 19, 19),	/* black */
    };

    unsigned char *y_mem = (yuv->order & YUV_YC) ? mem : mem + 1;
    unsigned char *c_mem = (yuv->order & YUV_CY) ? mem : mem + 1;
    unsigned int u = (yuv->order & YUV_YCrCb) ? 2 : 0;
    unsigned int v = (yuv->order & YUV_YCbCr) ? 2 : 0;
    unsigned int x;
    unsigned int y;

    /* Luma */
    for(y = 0; y < height * 6 / 9; ++y)
    {
        for(x = 0; x < width; ++x)
            y_mem[2 * x] = colors_top[x * 7 / width].y;
        y_mem += stride;
    }

    for(; y < height * 7 / 9; ++y)
    {
        for(x = 0; x < width; ++x)
            y_mem[2 * x] = colors_middle[x * 7 / width].y;
        y_mem += stride;
    }

    for(; y < height; ++y)
    {
        for(x = 0; x < width * 5 / 7; ++x)
            y_mem[2 * x] = colors_bottom[x * 4 / (width * 5 / 7)].y;
        for(; x < width * 6 / 7; ++x)
            y_mem[2 * x] = colors_bottom[(x - width * 5 / 7) * 3
                                         / (width / 7) + 4].y;
        for(; x < width; ++x)
            y_mem[2 * x] = colors_bottom[7].y;
        y_mem += stride;
    }

    /* Chroma */
    for(y = 0; y < height * 6 / 9; ++y)
    {
        for(x = 0; x < width; x += 2)
        {
            c_mem[2 * x + u] = colors_top[x * 7 / width].u;
            c_mem[2 * x + v] = colors_top[x * 7 / width].v;
        }
        c_mem += stride;
    }

    for(; y < height * 7 / 9; ++y)
    {
        for(x = 0; x < width; x += 2)
        {
            c_mem[2 * x + u] = colors_middle[x * 7 / width].u;
            c_mem[2 * x + v] = colors_middle[x * 7 / width].v;
        }
        c_mem += stride;
    }

    for(; y < height; ++y)
    {
        for(x = 0; x < width * 5 / 7; x += 2)
        {
            c_mem[2 * x + u] = colors_bottom[x * 4 / (width * 5 / 7)].u;
            c_mem[2 * x + v] = colors_bottom[x * 4 / (width * 5 / 7)].v;
        }
        for(; x < width * 6 / 7; x += 2)
        {
            c_mem[2 * x + u] = colors_bottom[(x - width * 5 / 7) *
                                             3 / (width / 7) + 4].u;
            c_mem[2 * x + v] = colors_bottom[(x - width * 5 / 7) *
                                             3 / (width / 7) + 4].v;
        }
        for(; x < width; x += 2)
        {
            c_mem[2 * x + u] = colors_bottom[7].u;
            c_mem[2 * x + v] = colors_bottom[7].v;
        }
        c_mem += stride;
    }
}


static void fill_smpte(const UtilFormatInfo_t *info, void *planes[3],
                       unsigned int width, unsigned int height,
                       unsigned int stride)
{
    unsigned char *u, *v;

    switch(info->format)
    {
        case DRM_FORMAT_YUYV:
        case DRM_FORMAT_YVYU:
            return fill_smpte_yuv_packed(&info->yuv, planes[0], width,
                                         height, stride);

        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV21:
            u = info->yuv.order & YUV_YCbCr ? planes[1] : planes[1] + 1;
            v = info->yuv.order & YUV_YCrCb ? planes[1] : planes[1] + 1;
            return fill_smpte_yuv_planar(&info->yuv, planes[0], u, v,
                                         width, height, stride);
    }
}

static void fill_tiles_yuv_planar(const UtilFormatInfo_t *info,
                                  unsigned char *y_mem, unsigned char *u_mem,
                                  unsigned char *v_mem, unsigned int width,
                                  unsigned int height, unsigned int stride)
{
    const UtilYuvInfo_t *yuv = &info->yuv;
    unsigned int cs = yuv->chroma_stride;
    unsigned int xsub = yuv->xsub;
    unsigned int ysub = yuv->ysub;
    unsigned int x;
    unsigned int y;

    for(y = 0; y < height; ++y)
    {
        for(x = 0; x < width; ++x)
        {
            div_t d = div(x + y, width);
            uint32_t rgb32 = 0x00130502 * (d.quot >> 6)
                             + 0x000a1120 * (d.rem >> 6);
            struct color_yuv color =
                MAKE_YUV_601((rgb32 >> 16) & 0xff,
                             (rgb32 >> 8) & 0xff, rgb32 & 0xff);

            y_mem[x] = color.y;
            u_mem[x / xsub * cs] = color.u;
            v_mem[x / xsub * cs] = color.v;
        }

        y_mem += stride;
        if((y + 1) % ysub == 0)
        {
            u_mem += stride * cs / xsub;
            v_mem += stride * cs / xsub;
        }
    }
}

static void fill_tiles_yuv_packed(const UtilFormatInfo_t *info,
                                  void *mem, unsigned int width,
                                  unsigned int height, unsigned int stride)
{
    const UtilYuvInfo_t *yuv = &info->yuv;
    unsigned char *y_mem = (yuv->order & YUV_YC) ? mem : mem + 1;
    unsigned char *c_mem = (yuv->order & YUV_CY) ? mem : mem + 1;
    unsigned int u = (yuv->order & YUV_YCrCb) ? 2 : 0;
    unsigned int v = (yuv->order & YUV_YCbCr) ? 2 : 0;
    unsigned int x;
    unsigned int y;

    for(y = 0; y < height; ++y)
    {
        for(x = 0; x < width; x += 2)
        {
            div_t d = div(x + y, width);
            uint32_t rgb32 = 0x00130502 * (d.quot >> 6)
                             + 0x000a1120 * (d.rem >> 6);
            struct color_yuv color =
                MAKE_YUV_601((rgb32 >> 16) & 0xff,
                             (rgb32 >> 8) & 0xff, rgb32 & 0xff);

            y_mem[2 * x] = color.y;
            c_mem[2 * x + u] = color.u;
            y_mem[2 * x + 2] = color.y;
            c_mem[2 * x + v] = color.v;
        }

        y_mem += stride;
        c_mem += stride;
    }
}


static void fill_tiles(const UtilFormatInfo_t *info, void *planes[3],
                       unsigned int width, unsigned int height,
                       unsigned int stride)
{
    unsigned char *u, *v;

    switch(info->format)
    {
        case DRM_FORMAT_YUYV:
        case DRM_FORMAT_YVYU:
            return fill_tiles_yuv_packed(info, planes[0],
                                         width, height, stride);

        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV21:
            u = info->yuv.order & YUV_YCbCr ? planes[1] : planes[1] + 1;
            v = info->yuv.order & YUV_YCrCb ? planes[1] : planes[1] + 1;
            return fill_tiles_yuv_planar(info, planes[0], u, v,
                                         width, height, stride);
    }
}

static void fill_plain(const UtilFormatInfo_t *info, void *planes[3],
                       unsigned int width, unsigned int height,
                       unsigned int stride)
{
    memset(planes[0], 0x77, stride * height);
}

/*
 * UtilFillPattern - Fill a buffer with a test pattern
 * @format: Pixel format
 * @pattern: Test pattern
 * @planes: Array of buffers
 * @width: Width in pixels
 * @height: Height in pixels
 * @stride: Line stride (pitch) in bytes
 *
 * Fill the buffers with the test pattern specified by the pattern parameter.
 * Supported formats vary depending on the selected pattern.
 */
void UtilFillPattern(uint32_t format, UtilFillPattern_e pattern,
                     void *planes[3], unsigned int width,
                     unsigned int height, unsigned int stride)
{
    const UtilFormatInfo_t *info;

    info = util_format_info_find(format);
    if(info == NULL)
        return;

    switch(pattern)
    {
        case UTIL_PATTERN_TILES:
            return fill_tiles(info, planes, width, height, stride);

        case UTIL_PATTERN_SMPTE:
            return fill_smpte(info, planes, width, height, stride);

        case UTIL_PATTERN_PLAIN:
            return fill_plain(info, planes, width, height, stride);

        default:
            printf("Error: unsupported test pattern %u.\n", pattern);
            break;
    }
}