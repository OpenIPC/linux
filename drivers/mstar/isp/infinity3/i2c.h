#ifndef _I2C_H
#define _I2C_H

#ifndef PAD16BITS
#define PAD16BITS u32:16
#endif

struct __i2c_handle_t;

typedef struct {
    unsigned short reg;
    unsigned short data;
} __attribute__((packed, aligned(1))) I2C_ARRAY;

/*
typedef enum {
    I2C_RW_R,
    I2C_RW_W,
} I2C_RW;

typedef struct {
    I2C_RW  rw;
    unsigned short reg;
    unsigned short data;
} __attribute__((packed, aligned(1))) I2C_CPX_ARRAY;
*/

typedef enum {
    I2C_FMT_A8D8,
    I2C_FMT_A16D8,
    I2C_FMT_A8D16,
    I2C_FMT_A16D16,
    I2C_FMT_END
} ISP_I2C_FMT;

typedef enum {
    I2C_LEGACY_MODE,
    I2C_NORMAL_MODE
} ISP_I2C_MODE;


typedef struct {
    ISP_I2C_MODE        mode;
    ISP_I2C_FMT         fmt;
    u32                 speed;
    u16                 address;
    u16 reserved;
} __attribute__((packed, aligned(1))) app_i2c_cfg;

 ///////////////////////isp i2c///////////////////////
typedef struct {
  u32 reg_i2c_en                  :1 ; //h78
  u32 /* reserved */              :7;
  u32 reg_i2c_rstz                :1 ; //h78
  u32 /* reserved */              :7;
  PAD16BITS;

  u32 reg_sen_m2s_2nd_reg_adr     :8 ; //h79
  u32 /* reserved */              :8;
  PAD16BITS;

  u32 reg_sen_m2s_sw_ctrl         :1 ; //h7a
  u32 reg_sen_m2s_mode            :1 ; //h7a
  u32 reg_sen_m2s_cmd_bl          :1 ; //h7a
  u32 reg_sen_m2s_reg_adr_mode    :1 ; //h7a
  u32 /* reserved */              :10;
  u32 reg_sda_i                   :1 ; //h7a
  u32 reg_m2s_status              :1 ; //h7a
  PAD16BITS;

  u32 reg_sen_m2s_cmd             :16; //h7b
  PAD16BITS;

  u32 reg_sen_m2s_rw_d            :16; //h7c
  PAD16BITS;

  u32 reg_sen_prescale            :10; //h7d
  u32 /* reserved */              :6;
  PAD16BITS;

  u32 reg_sen_sw_scl_oen          :1 ; //h7e
  u32 reg_sen_sw_sda_oen          :1 ; //h7e
  u32 /* reserved */              :14;
  PAD16BITS;

  u32 reg_sen_rd                  :16; //h7f
  PAD16BITS;
} __attribute__((packed, aligned(1))) isp7_i2c_cfg_t;

#define I2C_REQ_STATUS_WAIT 0x00
#define I2C_REQ_STATUS_DONE 0x01
#define I2C_REQ_STATUS_ERROR 0x02
typedef struct
{
  u32 tag;
  u32 status;
  u32 mode;
  u32 fmt;
  u32 speed;
  u32 slave_addr;
  u32 active_frame_id; //the frame number which will apply this data
  u32 ndata;
  I2C_ARRAY data[16];
}__attribute__((packed, aligned(1))) i2c_request;

typedef struct __i2c_handle_t {
    int version;
#if 0
    pi2c_setmode       i2c_setmode;
    pi2c_setfmt        i2c_setfmt;
    pi2c_setspeed      i2c_setspeed;
    pi2c_tx            i2c_tx;
    pi2c_rx            i2c_rx;
    pi2c_array_tx      i2c_array_tx;
    pi2c_array_tx2     i2c_array_tx2;
    pi2c_array_rx      i2c_array_rx;
    pi2c_array_rx2     i2c_array_rx2;
    pi2c_array_rw      i2c_array_rw;
    pi2c_array_rw2     i2c_array_rw2;
    pi2c_get_status    i2c_get_status;
    pi2c_reset         i2c_reset;
    pi2c_enable        i2c_enable;
    pi2c_settimeout    i2c_settimeout;
#endif
    int                timeout;
    volatile isp7_i2c_cfg_t  *i2c_reg;
    i2c_request req;
    //MSYS_DMEM_INFO req_buf_info;
} __attribute__((packed, aligned(1))) i2c_handle_t;

i2c_handle_t* isp_i2c_init(u32 virt_reg_base);
int isp_i2c_release(i2c_handle_t* handle);
int isp_i2c_array_tx(i2c_handle_t* handle, app_i2c_cfg* cfg, I2C_ARRAY *pdata, int ndata);
int isp_i2c_request(i2c_handle_t* handle,i2c_request *req);
#endif
