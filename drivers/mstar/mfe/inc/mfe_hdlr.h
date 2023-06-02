
#ifndef _MFE_HDLR_H_
#define _MFE_HDLR_H_

typedef struct {
    int i_idx;
    int b_busy;
    uint base_y, base_c;
} frame_t;

typedef struct {
    int i_idx;
    int i_poc;
    int b_use;
    uint32 planes[2];
} fs_t;

typedef struct {
    int i_total;
    fs_t frames[8];
} vbstore_t;

typedef struct {
    void* mem_vptr;
    int size;
    dma_addr_t bus_addr;
    unsigned int miu_addr;
} kma_t;

typedef struct {
    uint32 addr;
    void* vptr;
    int size;
} mem_t;

typedef enum {
    speed_invalid,
    speed_lowest,
    speed_low,
    speed_high,
    speed_highest,
} clock_speed;

typedef enum {
    pixel_tile_8x8,
    pixel_tile_16x32,
    pixel_semi_planer,
} pixel_format;

typedef struct mfe_hdlr {
    /* corresponding mfe-device */
    mfe_asic* asic;
    /* rate control */
    cvbr_rc cvbr;
    /* mfe-registers */
    mfe_regs* regs;

    struct mutex stream_lock;
    struct mutex encode_lock;

    int b_stream;
    int i_encode;

    int i_codec_type;
    clock_speed  clk_speed;
    pixel_format pixel_fmt;
    /* video input parameters */
    int i_picture_w, i_picture_h;
    int i_display_w, i_display_h;
    int i_mb_w, i_mb_h, i_mb_wxh;
    /* picture type decision */
    int i_seq_numb;
    int i_num_ppic, i_num_bpic;
    int i_cnt_ppic, i_cnt_bpic;
    /* motion search */
    int i_dmv_x;
    int i_dmv_y;
    int i_subpel;
    unsigned int i_mv_block[2];
    /* bit-rate */
    int i_method;
    int i_bps;
    int i_qp;
    /* frame-rate */
    frac_t fr_fps;
    /* avc syntax element */
    sps_t sps;
    pps_t pps;
    slice_t sh;
    /* freeback and statistic */
    int i_stat_size;
    int i_last_bits;
    int i_pic_avgqp;

    kma_t kma;

    uint32 i_base_gn;
    uint32 i_size_gn;
    uint32 i_base_bs;
    uint32 i_size_bs;
    void*  p_vptr_bs;

    int i_num_ref_frames;
    frame_t* dpb[4];
    frame_t frames[4];
    frame_t* recon;
    frame_t  input;
    long long i_timecode;

    uchar code_sh[64];
    int   bits_sh;
    uchar code_sets[64];
    int   bits_sets;

    void* p_outbuf;
    int   i_outlen;
    int   i_uselen;
    int   i_remlen;

} mfe_hdlr;

int  mfe_hdlr_request(void**);
int  mfe_hdlr_release(void* );

int  mfe_hdlr_seq_init(mfe_hdlr* h);
int  mfe_hdlr_seq_exit(mfe_hdlr* h);
void mfe_hdlr_pic_mode(mfe_hdlr* h);
void mfe_hdlr_pic_init(mfe_hdlr* h);
void mfe_hdlr_pic_done(mfe_hdlr* h);
uint mfe_hdlr_pic_wait(void);
void mfe_hdlr_pic_read_bits(mfe_hdlr* h);
void mfe_hdlr_pic_prep_regs(mfe_hdlr* h);

#endif//_MFE_HDLR_H_

