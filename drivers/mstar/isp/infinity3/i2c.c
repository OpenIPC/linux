#include <linux/module.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include "ms_msys.h"
#include "i2c.h"


#define I2C_BUSY 1
#define I2C_IDLE 0
typedef struct
{
  unsigned long al;
  unsigned long ah;
}addr64;
i2c_handle_t* isp_i2c_init(u32 virt_reg_base)
{
  i2c_handle_t* handle = (i2c_handle_t*) kzalloc(sizeof(i2c_handle_t),GFP_KERNEL);
  handle->i2c_reg = (isp7_i2c_cfg_t*) virt_reg_base;
  return handle;
}

int isp_i2c_release(i2c_handle_t* handle)
{
  kfree(handle);
  return 0;
}

 static int wait_i2c_done(i2c_handle_t* handle, int ms)
{
    int status = 1, count = 0, interval = 20; // us
    int timeout = ms/interval;
    timeout = 625000000; // 1000ms@600MHz
    do {
        status = handle->i2c_reg->reg_m2s_status;
        if (status != I2C_BUSY)
            break;
        count++;
    } while (status == I2C_BUSY && count < timeout);

    if (count == timeout){
        pr_debug("%d: I2C Timeout!\n", ms);
        return -1;
    } else
        return 0;
}

int isp_i2c_array_tx(i2c_handle_t* handle, app_i2c_cfg* cfg, I2C_ARRAY *pdata, int ndata) {
    unsigned short i, reg, rc, data;
    short rw_addr = cfg->address;
    volatile isp7_i2c_cfg_t  *i2c_reg = handle->i2c_reg;

    for (i = 0; i < ndata; i++) {
        reg = pdata[i].reg;
        data = pdata[i].data;

        switch(cfg->fmt) {
        default:
        case I2C_FMT_A8D8:
            i2c_reg->reg_sen_m2s_rw_d = data&0xff;
            i2c_reg->reg_sen_m2s_2nd_reg_adr = 0;
            i2c_reg->reg_sen_m2s_cmd = (rw_addr) | ((reg&0xff) << 8);
            break;
        case I2C_FMT_A16D8:
            i2c_reg->reg_sen_m2s_rw_d = data&0xff;
            i2c_reg->reg_sen_m2s_2nd_reg_adr = reg&0x00ff;
            i2c_reg->reg_sen_m2s_cmd = (rw_addr) | (reg&0xff00);
            break;
        case I2C_FMT_A8D16:
            i2c_reg->reg_sen_m2s_rw_d = ((data&0xff00) >> 8) | ((data&0x00ff) << 8);
            i2c_reg->reg_sen_m2s_2nd_reg_adr = 0;
            i2c_reg->reg_sen_m2s_cmd = (rw_addr) | ((reg&0xff) << 8);
            break;
        case I2C_FMT_A16D16:
            i2c_reg->reg_sen_m2s_rw_d = ((data&0xff00) >> 8) | ((data&0x00ff) << 8);
            i2c_reg->reg_sen_m2s_2nd_reg_adr = reg&0x00ff;
            i2c_reg->reg_sen_m2s_cmd = (rw_addr) | (reg&0xff00);
            break;

        }

        rc = wait_i2c_done(handle, handle->timeout);

    }

    return 0;

}

int isp_i2c_request(i2c_handle_t* handle,i2c_request *req)
{
  app_i2c_cfg cfg;
  cfg.address = req->slave_addr;
  cfg.fmt = req->fmt;
  cfg.speed = req->speed;
  cfg.mode = req->mode;
  if(!isp_i2c_array_tx(handle,&cfg,req->data,req->ndata))
  {
    req->status = I2C_REQ_STATUS_DONE;
    return 0;
  }
  else
  {
    req->status = I2C_REQ_STATUS_ERROR;
    return -1;
  }
}