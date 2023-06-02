/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/

#ifndef SW_PICTURE_H
#define SW_PICTURE_H

#include "base_type.h"
#include "queue.h"
#include "sw_parameter_set.h"
#include "container.h"
#include "encasiccontroller.h"



#define MAX_CU_WIDTH 64

#define RECON_PIPELINE_CTU_NUM 4

enum sw_image_type
{
  SW_IMAGE_U8 = 0,
  SW_IMAGE_I32
};



struct sw_data
{
  enum sw_image_type type;
  i32 lum_width;    /* Width of *lum */
  i32 lum_height;   /* Height of *lum */
  i32 ch_width;   /* Width of *cb and *cr */
  i32 ch_height;    /* Height of *cb and *cr */
  i32 lum_pad_width;
  i32 lum_pad_height;
  i32 ch_pad_width;
  i32 ch_pad_height;
  u32 lum;
  u32 cb;
  u32 cr;
};


struct sw_buffer_s
{
  u8 *stream;         /* Stream buffer */
  i32 startOffset;    /* Bytes in stream header remainder */
  u32 hdrRemainder1;  /* Stream header remainder MSB */
  u32 hdrRemainder2;  /* Stream header remainder LSB */
  i32 size;           /* Size of stream buffer (user set) */
  i32 *table;
  i32 tableSize;
  i32 tableCnt;
} ;

struct sw_compress
{
  u32 lumaCompressed;
  u32 chromaCompressed;
  u32 lumaTblBase;
  u32 chromaTblBase;
};

struct sw_picture
{
  struct node *next;
  struct queue memory;
  struct sw_data input; /* Input image store */
  struct sw_data recon; /* Recon image store */

  struct queue slice; /* Slice store */
  struct slice *sliceInst;
  struct vps *vps;  /* Active video parameter set */
  struct sps *sps;  /* Active sequence parameter set */
  struct pps *pps;  /* Active picture parameter set */
  struct rps *rps;  /* Active reference picture set */
  struct sw_picture ** *rpl; /* Reference picture lists */
  u32 recon_4n_base;

  i32 poc;    /* Picture order count */
  i32 ref_cnt;    /* Reference cnt (other thread may use) */
  i32 reference;    /* Picture is reference picture */

  i32 column;
  i32 row;

  i32 ctb_num;

  i32 picture_memeory_id;
  i32 picture_memeory_init;


  i32 slice_deblocking_filter_override_flag;
  i32 disableDeblocking;
  i32 sliceNum;
  i32 sliceSize;

  i32 trace_pic_cnt; /* for trace only */

  struct sw_buffer_s buffer;

  //compress
  struct sw_compress recon_compress;
};


void create_slices_ctrlsw(struct sw_picture *p, struct pps *pps, u32 sliceSize);

struct hevc_instance;
struct sw_picture *create_picture_ctrlsw(struct hevc_instance *instance, struct vps *vps, struct sps *sps,
    struct pps *pps, u32 sliceSize, u32 srcWidth, u32 srcHeight);
struct sw_picture *get_picture(struct container *c, i32 poc);
void sw_free_pictures(struct container *c);
void reference_picture_list(struct container *c, struct sw_picture *pic);

void sw_free_picture(struct sw_picture *p);
#endif
