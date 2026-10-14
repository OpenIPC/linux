#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <plat-anyka/ak_isp_drv.h>
#include "ak39_isp2.h"
#include "ak39_isp2_reg.h"
#include "ak39_isp2_3a.h"

#define DMA_CONFIG_REG_SIZE			3540
#define DMA_STAT_DATA_SIZE			4800
#define STATIC_DEAD_PIXELS_SIZE		(1024*4)

/* each register block bytes */
#ifdef CONFIG_CPU_AK3918EV200
/*
 * EV200's register blocks are smaller: it has two output channels where the
 * EV300 parts have three. These are the sizes this SDK's own comments still
 * state, and they match the
 * sizes the EV200 ISP actually uses.
 *
 * The word indices inside the blocks move with the sizes; they are named in
 * ak39_isp2_reg.h rather than written as literals here.
 */
#define REG_BLK1_SIZE				236	/* 59 words, EV300 68 */
#define REG_BLK2_SIZE				208	/* 52, unchanged */
#define REG_BLK3_SIZE				168	/* 42, unchanged */
#define REG_BLK4_SIZE				140	/* 35, EV300 52 */
#define REG_BLK5_SIZE				132	/* 33, EV300 36 */
#else
#define REG_BLK1_SIZE				272
#define REG_BLK2_SIZE				208
#define REG_BLK3_SIZE				168
#define REG_BLK4_SIZE				208
#define REG_BLK5_SIZE				144
#endif

/* each memory block bytes */
#define MEM_BLK1_SIZE				512
#define MEM_BLK2_SIZE				512
#define MEM_BLK3_SIZE				172
#define MEM_BLK4_SIZE				512
#define MEM_BLK5_SIZE				576
#define MEM_BLK6_SIZE				256

//#define ISP_DEBUG
#ifdef ISP_DEBUG
#define isp_dbg(stuff...)       printk("ISP: " stuff)
#else
#define isp_dbg(stuff...)   do{}while(0)
#endif

/* Always on, and deliberately sparse: these lines are what tell us on hardware
 * which variant of the driver is actually running. */
#define isp_info(stuff...)      printk(KERN_INFO stuff)

//#undef REG32 //add by xc
#define REG32(_reg_)  (*(volatile unsigned long *)(_reg_))

int _isp_update_isp_d_gain(T_U32 isp_d_gain);

AK_ISP_STRUCT *isp=NULL;

//static int printk_flag = 0;
//static int ccm_printk_flag = 0;

void isp2_print_reg_table(void) 
{
	unsigned int i = 0;
	unsigned int num = 0;
	unsigned int cmd;
	 
	cmd = 0;
	cmd =  REG32(isp->base+0x00);
	printk("reg0x00 = %x\n",cmd);

	cmd = 0;
	cmd =  REG32(isp->base+0x04);
	printk("reg0x04 = %x\n",cmd);
	 
	cmd = 0;
	cmd =  REG32(isp->base+0x08);
	printk("reg0x08 = %x\n",cmd);
	 
	cmd = 0;
	cmd =  REG32(isp->base+0x0c);
	printk("reg0x0c = %x\n",cmd);
	 
	cmd = 0;
	cmd =  REG32(isp->base+0x10);
	printk("reg0x10 = %x\n",cmd);
	 
	cmd = 0;
	cmd =  REG32(isp->base+0x14);
	printk("reg0x14 = %x\n",cmd);
	 
	cmd = 0;
	cmd =  REG32(isp->base+0x18);
	printk("reg0x18 = %x\n",cmd);
	 
	cmd = 0;
	cmd =  REG32(isp->base+0x1c);
	printk("reg0x1c = %x\n",cmd);
	 
	cmd = 0;
	printk("\n\n\n");
	 
	
	/* register block1, 236 bytes */
	num = (REG_BLK1_SIZE / 4);
	REG32(isp->base+REG_BLOCK_1)=0x01;
	for(i=0; i<num; i++)
	{
		cmd = REG32(isp->base+REG_BLOCK_1);
		printk("REGBLK1 reg0x%X[%03d] = %.8x:%.8x\n",
			REG_BLOCK_1, i, cmd, isp->reg_blkaddr1[i]);
	}
	printk("\n\n\n");

	/* register block2, 208 bytes */
	num = (REG_BLK2_SIZE / 4);
	REG32(isp->base+REG_BLOCK_2)=0x01;
	for(i=0; i<num; i++)
	{
		cmd =REG32(isp->base+REG_BLOCK_2);
		printk("REGBLK2 reg0x%X[%03d] = %.8x:%.8x\n",
			REG_BLOCK_2, i, cmd, isp->reg_blkaddr2[i]);
	}
	printk("\n\n\n");
	
	/* memory block1, 512 bytes */
	num = (MEM_BLK1_SIZE / 4);
	REG32(isp->base+MEM_BLOCK_1)=0x01;
	for(i=0; i<num; i++)
	{
		cmd = REG32(isp->base+MEM_BLOCK_1);
		printk("MEMBLK1 reg0x%X[%03d] = %.8x:%.8x\n",
			MEM_BLOCK_1, i, cmd, isp->cfg_mem_blkaddr1[i]);
	} 
	printk("\n\n\n");
	
	/* memory block2, 512 bytes */
	num = (MEM_BLK2_SIZE / 4);
	REG32(isp->base+MEM_BLOCK_2) =0x01; 
	for(i=0; i<num; i++)
	{
		cmd = REG32(isp->base+MEM_BLOCK_2);
		printk("MEMBLK2 reg0x%X[%03d] = %.8x:%.8x\n",
			MEM_BLOCK_2, i, cmd, isp->cfg_mem_blkaddr2[i]);
	}
	printk("\n\n\n");

	/* memory block3, 172 bytes */
	num = (MEM_BLK3_SIZE / 4);
	REG32(isp->base+MEM_BLOCK_3) =0x01;
	for(i=0; i<num; i++)
	{
		cmd = REG32(isp->base+MEM_BLOCK_3);
		printk("MEMBLK3 reg0x%X[%03d] = %.8x:%.8x\n",
			MEM_BLOCK_3, i, cmd, isp->cfg_mem_blkaddr3[i]);
	}
	printk( "\n\n\n");
	 
	/* memory block4, 512 bytes */
	num = (MEM_BLK4_SIZE / 4);
	REG32(isp->base+MEM_BLOCK_4) =0x01;
	for(i=0; i<num; i++)
	{
		cmd = REG32(isp->base+MEM_BLOCK_4);
		printk( "MEMBLK4 reg0x%X[%03d] = %.8x:%.8x\n",
			MEM_BLOCK_4, i, cmd, isp->cfg_mem_blkaddr4[i]);
	}
	printk("\n\n\n");

	/* memory block5, 576 bytes */
	num = (MEM_BLK5_SIZE / 4);
	REG32(isp->base+MEM_BLOCK_5) =0x01;
	for(i=0; i<num; i++)
	{
		cmd = REG32(isp->base+MEM_BLOCK_5);
		printk( "MEMBLK5 reg0x%X[%03d] = %.8x:%.8x\n",
			MEM_BLOCK_5, i, cmd, isp->cfg_mem_blkaddr5[i]);
	}
	printk("\n\n\n");

	/* memory block6, 256 bytes */
	num = (MEM_BLK6_SIZE / 4);
	REG32(isp->base+MEM_BLOCK_6) =0x01;
	for(i=0; i<num; i++)
	{
		cmd = REG32(isp->base+MEM_BLOCK_6);
		printk( "MEMBLK6 reg0x%X[%03d] = %.8x:%.8x\n",
			MEM_BLOCK_6, i, cmd, isp->cfg_mem_blkaddr6[i]);
	}
	printk("\n\n\n");

	/* register block3, 168 bytes */
	num = (REG_BLK3_SIZE / 4);
	REG32(isp->base+REG_BLOCK_3) =0x01;
	for(i=0; i<num; i++)
	{
		cmd = REG32(isp->base+REG_BLOCK_3);
		printk("REGBLK3 reg0x%X[%03d] = %.8x:%.8x\n",
			REG_BLOCK_3, i, cmd, isp->reg_blkaddr3[i]);
	}
	printk("\n\n\n");

	/* register  block4, 140 bytes */
	num = (REG_BLK4_SIZE / 4);
	REG32(isp->base+REG_BLOCK_4) = 0x01;
	for(i=0; i<num; i++)
	{
		cmd =REG32(isp->base+REG_BLOCK_4);
		printk("REGBLK4 reg0x%X[%03d] = %.8x:%.8x\n",
			REG_BLOCK_4, i, cmd, isp->reg_blkaddr4[i]);
	}
	printk("\n\n\n");

	/* register  block5, 132 bytes */
	num = (REG_BLK5_SIZE / 4);
	REG32(isp->base+REG_BLOCK_5) = 0x01;
	for(i=0; i<num; i++)
	{
		cmd = REG32(isp->base+REG_BLOCK_5);
		printk( "REGBLK5 reg0x%X[%03d] = %.8x:%.8x\n",
			REG_BLOCK_5, i, cmd, isp->reg_blkaddr5[i]);
	}
	printk("\n\n\n");    
}

/* EV200's frame-buffer enables count up, so its buffer-id nibble needs no
 * reversal; the call sites are compiled out under EV200 accordingly. */
#ifndef CONFIG_CPU_AK3918EV200
static unsigned char low_4bits_revert(unsigned char v)
{
	unsigned char _t = (((v) & 0x5) << 1) | (((v) >> 1) & 0x5);
	_t = ((_t & 0x3) << 2) | ((_t >> 2) & 0x3);
	return _t;
}
#endif

int ak_isp_vi_set_input_size(int width, int height)
{
    unsigned int err_check;

    if (!isp)
        return -1;

    err_check = REG32(isp->base + 0x4);
    CLEAR_BITS(err_check, 0, 28);
    err_check |= (width<<EXP_HOR)|(height<<EXP_VERT);
    REG32(isp->base + CSI_ERR_CHECK_1-ISP_BASE_ADDR) = err_check;
    return 0;
}

int ak_isp_vi_set_sub128(int sub)
{
	unsigned int cmd;

	cmd = REG32(isp->base + CSI_ERR_CHECK_1);
	CLEAR_BITS(cmd, 30, 1);
	cmd |= sub ? (1<<30):0;
	REG32(isp->base + CSI_ERR_CHECK_1) = cmd;

	return 0;
}

int ak_isp_vo_check_irq_status (void)
{
    unsigned int irq_status;
    if(!isp)
    	return 0;
    
    irq_status = REG32(isp->base + 0x10);

    return irq_status;
}

int ak_isp_vo_clear_irq_status(int bit)
{
    if (!isp)
        return -1;
    
    REG32(isp->base + 0x10) |= bit;//0xbfff;
    return 0;
}

int ak_isp_vo_enable_irq_status(int bit)
{
	if (!isp)
        	return -1;
	REG32(isp->base + 0x14) &= ~(0xfffff);//0x01;//0xffff;
	REG32(isp->base + 0x14) |= bit;//0x01;//0xffff;
	return 0;
}

int ak_isp_vo_set_cfg_reg(int regaddr, int value, int bitmask)
{
    int bits;

    switch(regaddr)
    {
    case 0x0:
        bitmask &=0x3e0000ff;
        break;
    case 0x04:
    case 0x08:
    case 0x0c:
        break;
    default:
        return 0;
    }
    
    bits = REG32(isp->base + regaddr);
    bits &= ~bitmask;
    bits |= value&bitmask;
    REG32(isp->base + regaddr) = bits;

    return 1;
}

AK_ISP_PCLK_POLAR ak_isp_get_pclk_polar(void)
{
	if (!isp)
		return POLAR_ERR;

	if (isp->misc_para.pclk_pol) {
		return POLAR_FALLING;
	}

	return POLAR_RISING;
}

int ak_isp_vo_get_inputdataw(void)
{
	int bits;
	unsigned int cmd;

	cmd = REG32(isp->base + GEN_CTRL_ADDR);
	cmd >>= DATA_WIDTH;
	cmd &= 0x3;
	switch (cmd) {
		case 0:
			bits = 12;
			break;
		case 1:
			bits = 10;
			break;
		case 2:
			bits = 8;
			break;
		default:
			bits = 0;
			break;
	}

	return bits;
}

int _isp_set_misc_attr(void)
{
	int tmp;
    unsigned int cmd = 0;
    isp_dbg("%s enter.\n", __func__);
    
    cmd = REG32(isp->base + GEN_CTRL_ADDR);
    CLEAR_BITS(cmd, 4, 4);
    CLEAR_BITS(cmd, 25, 5);
    CLEAR_BITS(cmd, 0, 2);
    cmd |= (LOW_BITS(isp->misc_para.cfa_mode, 2) << RAW_SEQ)
    	|  (LOW_BITS(isp->misc_para.hsyn_pol, 1) << HERF_POL)
    	|  (LOW_BITS(isp->misc_para.vsync_pol, 1) << VSYNC_POL)
    	|  (LOW_BITS(isp->misc_para.test_pattern_en, 1) << TEST_PATTERN_EN)
    	|  (LOW_BITS(isp->misc_para.test_pattern_cfg, 4) << TEST_PATTERN)
    	|  (LOW_BITS(isp->misc_para.inputdataw, 2) << DATA_WIDTH);

   tmp = isp->misc_para.inputdataw & 0x3;
   if (tmp == 0)
	   cmd |= 0x1 << DATA_WIDTH; //10bits input;
   else if (tmp==1)
	   cmd |= 0x2 << DATA_WIDTH; //8bits input;
   else if (tmp==2)
	   cmd |= 0x0 << DATA_WIDTH; //12bits input;

   /*
    * One bit per output channel, and on this part 1 is not what the rest
    * of the pipeline expects: the encoder reads the chroma of a frame
    * written this way as two half-size planes, so it takes the
    * interleaved bytes as U followed by V. Luma is untouched, chroma
    * becomes noise that averages neutral, and the picture comes out
    * grey-green with every shape in the right place - which is what it
    * did until this was found by clearing the bits on a running camera.
    *
    * Verified live: with these clear, both the main and the sub stream
    * encode in colour. Channel 3 does not exist here.
    */
#ifdef CONFIG_CPU_AK3918EV200
	CLEAR_BITS(cmd, MAIN_UV_MODE, 3);
#else
   /*uv planar*/
	cmd |= 0x7 << MAIN_UV_MODE;
#endif

    REG32(isp->base + GEN_CTRL_ADDR) = cmd;

    cmd = REG32(isp->base + CSI_ERR_CHECK_2);
    CLEAR_BITS(cmd, 16, 16);
    cmd |= (LOW_BITS(isp->misc_para.one_line_cycle, 16) << ONE_LINE_CYCLE);
    REG32(isp->base + CSI_ERR_CHECK_2) = cmd;

    cmd = REG32(isp->base + CSI_HANDLE_ADDR);
    CLEAR_BITS(cmd, 31, 1);
    CLEAR_BITS(cmd, 8, 16);
    CLEAR_BITS(cmd, 2, 6);
    cmd |= (LOW_BITS(isp->misc_para.frame_start_delay_en, 1) << FRAME_START_DELAY_EN)
    	|  (LOW_BITS(isp->misc_para.hblank_cycle, 16) << HBLANK_RAW_CYCLE)
    	|  (LOW_BITS(isp->misc_para.frame_start_delay_num, 6) << FRAME_START_DELAY_NUM);
    REG32(isp->base + CSI_HANDLE_ADDR) = cmd;

#ifndef CONFIG_CPU_AK3918EV200
	/*
	 * 0x1C is REG_UPDATE, the config-table upload trigger. This part has no
	 * MIPI line-end fields in its top byte and stock never writes them here.
	 */
	cmd = REG32(isp->base+0x1C);//lz0499 9_21
	CLEAR_BITS(cmd, 24, 8);
	cmd |= (LOW_BITS(isp->misc_para.mipi_line_end_sel, 1) << MIPI_LINE_END_SEL)
		|  (LOW_BITS(isp->misc_para.mipi_line_end_cnt_en_cfg, 1) << MIPI_LINE_END_CNT_EN)
		|  (LOW_BITS(isp->misc_para.mipi_count_time, 6) << MIPI_COUNT_TIME);
	REG32(isp->base+0x1C) = cmd;
#endif

	cmd = 0;
	cmd = isp->reg_blkaddr5[R5_ENABLE];
#ifdef CONFIG_CPU_AK3918EV200
	/* No TWOFRAME_MERGE_EN at bit 27 on this part - stock clears 25,26 only. */
	CLEAR_BITS(cmd, HFILP_EN, 2);
	cmd |= (LOW_BITS(isp->misc_para.mirror_en, 1) << HFILP_EN)
		|  (LOW_BITS(isp->misc_para.flip_en, 1) << VFILP_EN);
#else
	CLEAR_BITS(cmd, HFILP_EN, 3); //lz0499 9_12
	cmd |= (LOW_BITS(isp->misc_para.mirror_en, 1) << HFILP_EN)
		|  (LOW_BITS(isp->misc_para.flip_en, 1) << VFILP_EN)
		|  (LOW_BITS(isp->misc_para.twoframe_merge_en,1) <<  TWOFRAME_MERGE_EN);
#endif
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

    return 0;
}

int ak_isp_vo_set_misc_attr(AK_ISP_MISC_ATTR *p_misc)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->misc_para),p_misc,sizeof(AK_ISP_MISC_ATTR));
    
    return 0;
}

int ak_isp_vo_get_misc_attr(AK_ISP_MISC_ATTR *p_misc)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_misc,&(isp->misc_para),sizeof(AK_ISP_MISC_ATTR));

    return 0;
}

int ak_isp_set_flip_mirror(int flip_en, int mirror_en)
{
	unsigned int cmd = 0;

	isp_dbg("%s enter.\n", __func__);

	cmd = isp->reg_blkaddr5[R5_ENABLE];
	CLEAR_BITS(cmd, HFILP_EN, 2);
	cmd |= ((flip_en ? 1:0) << VFILP_EN)|
		((mirror_en ? 1:0) << HFILP_EN);
	isp->reg_blkaddr5[R5_ENABLE] = cmd;
	//printk("cmd:0x%x ,flip_en:%d, mirror_en:%d, enter.\n", cmd, flip_en, mirror_en);

	return 0;
}

void ak_isp_get_flip_mirror(int * flip_en, int * mirror_en, int * height_block_num)
{
	unsigned int cmd = 0;

	isp_dbg("%s enter.\n", __func__);

	cmd = isp->reg_blkaddr5[R5_ENABLE];
	if (cmd & (1 << VFILP_EN))
		*flip_en = 1;
	else 
		*flip_en = 0;
	if (cmd & (1 << HFILP_EN))
		*mirror_en = 1;
	else
		*mirror_en = 0;

	cmd = isp->reg_blkaddr3[20];
	if(((cmd>>TNR_HEIGHT_BLOCK_NUM) & 0x01) == TNR_HEIGHT_BLOCKSIZE_16)
		*height_block_num = 16;
	else
		*height_block_num = 24;
}

int ak_isp_vo_update_regtable( enum  reg_updata_mode  mode)
{
    switch(mode)
    {
    case MODE_LEAST:
         REG32(isp->base + 0x1c) = 0x1<<2 ;
         break;

    case MODE_MOST:
         REG32(isp->base + 0x1c) = 0x1<<1;
         break;
    case MODE_ALL:
         REG32(isp->base + 0x1c) = 0x1<<0 ;
         break;
         
    default:
        printk("error updata reg table mode\n");
        break;      
   }
   
   return 0;    
}

int ak_isp_vo_update_setting(void)
{
	if(!isp)
		return 0;
	
	isp->reg_update_flag = MODE_ALL;
	return 0;
}

int ak_isp_vo_update_setting_most(void)
{
	if(!isp)
		return 0;
	
	isp->reg_update_flag = MODE_MOST;
	return 0;
}

int  _isp_enable_buffer(void)
{
	unsigned long cmd = 0;

	cmd = REG32(isp->base + ISP_ADDR_EN_ADDR);
	if(isp->enable_buffer_list&0x1)
	{
		SET_BITS(cmd, MAIN_ADDR1_EN, 1);
		SET_BITS(cmd, SUB_ADDR1_EN, 1);
#ifndef CONFIG_CPU_AK3918EV200
		SET_BITS(cmd, CH3_ADDR1_EN, 1);
#endif
	}

	if(isp->enable_buffer_list&0x2)
	{
		SET_BITS(cmd, MAIN_ADDR2_EN, 1);
		SET_BITS(cmd, SUB_ADDR2_EN, 1);
#ifndef CONFIG_CPU_AK3918EV200
		SET_BITS(cmd, CH3_ADDR2_EN, 1);
#endif
	}

	if(isp->enable_buffer_list&0x4)
	{
		SET_BITS(cmd, MAIN_ADDR3_EN, 1);
		SET_BITS(cmd, SUB_ADDR3_EN, 1);
#ifndef CONFIG_CPU_AK3918EV200
		SET_BITS(cmd, CH3_ADDR3_EN, 1);
#endif
	}

	if(isp->enable_buffer_list&0x8)
	{
		SET_BITS(cmd, MAIN_ADDR4_EN, 1);
		SET_BITS(cmd, SUB_ADDR4_EN, 1);
#ifndef CONFIG_CPU_AK3918EV200
		SET_BITS(cmd, CH3_ADDR4_EN, 1);
#endif
	}

	REG32(isp->base + ISP_ADDR_EN_ADDR) = cmd;

	return 0;
}

int  ak_isp_vo_enable_buffer(enum buffer_id  id)
{
	if(!isp)
		return 0;
	
    switch(id)
    {
    case BUFFER_ONE:
        isp->enable_buffer_list |=0x1;
        break;
    case BUFFER_TWO:
        isp->enable_buffer_list |=0x2;
        break;
    case BUFFER_THREE:
        isp->enable_buffer_list |=0x4;
        break;
    case BUFFER_FOUR:
        isp->enable_buffer_list |=0x8;
        break;
    default:
        break;
    }
    isp_dbg("%s :id=%d, list=0x%x\n", __func__, id, isp->enable_buffer_list);

    return 0;
}

int  ak_isp_vo_disable_buffer(enum buffer_id  id)
{
    unsigned long cmd = 0;

    if(!isp)
	return 0;
    
    isp_dbg("%s enter:id=%d, en=0x%x\n", __func__, id,
            REG32(isp->base + ISP_ADDR_EN_ADDR));
    
    switch(id){
    case BUFFER_ONE:
        cmd = REG32(isp->base + ISP_ADDR_EN_ADDR);
        CLEAR_BITS(cmd, MAIN_ADDR1_EN, 1);
        CLEAR_BITS(cmd, SUB_ADDR1_EN, 1);
#ifndef CONFIG_CPU_AK3918EV200
		CLEAR_BITS(cmd, CH3_ADDR1_EN, 1);
#endif
        REG32(isp->base + ISP_ADDR_EN_ADDR) = cmd;
        CLEAR_BITS(isp->enable_buffer_list, 0, 1);
        break;
    case BUFFER_TWO:
        cmd = REG32(isp->base + ISP_ADDR_EN_ADDR);
        CLEAR_BITS(cmd, MAIN_ADDR2_EN, 1);
        CLEAR_BITS(cmd, SUB_ADDR2_EN, 1);
#ifndef CONFIG_CPU_AK3918EV200
		CLEAR_BITS(cmd, CH3_ADDR2_EN, 1);
#endif
        REG32(isp->base + ISP_ADDR_EN_ADDR) = cmd;
        CLEAR_BITS(isp->enable_buffer_list, 1, 1);
        break;
    case BUFFER_THREE:
        cmd = REG32(isp->base + ISP_ADDR_EN_ADDR);
        CLEAR_BITS(cmd, MAIN_ADDR3_EN, 1);
        CLEAR_BITS(cmd, SUB_ADDR3_EN, 1);
#ifndef CONFIG_CPU_AK3918EV200
		CLEAR_BITS(cmd, CH3_ADDR3_EN, 1);
#endif
        REG32(isp->base + ISP_ADDR_EN_ADDR) = cmd;
        CLEAR_BITS(isp->enable_buffer_list, 2, 1);
        break;
    case BUFFER_FOUR:
        cmd = REG32(isp->base + ISP_ADDR_EN_ADDR);
        CLEAR_BITS(cmd, MAIN_ADDR4_EN, 1);
        CLEAR_BITS(cmd, SUB_ADDR4_EN, 1);
#ifndef CONFIG_CPU_AK3918EV200
		CLEAR_BITS(cmd, CH3_ADDR4_EN, 1);
#endif
        REG32(isp->base + ISP_ADDR_EN_ADDR) = cmd;
        CLEAR_BITS(isp->enable_buffer_list, 3, 1);
        break;
    default:
        break;
    }
    
    return 0;
}

int ak_isp_vo_set_buffer_addr \
	(enum buffer_id  id,\
	      unsigned long yaddr_main_chan_addr, \
		  unsigned long yaddr_sub_chan_addr  \
		 /* unsigned long yaddr_chan3_addr*/ )//lz0499 9_26
{
#ifndef CONFIG_CPU_AK3918EV200
	isp_dbg("%s:id=%d, main_adr=%x, sub_addr=%x, ch3_chan_addr=%x\n", __func__, id, yaddr_main_chan_addr,  yaddr_sub_chan_addr, yaddr_chan3_addr);
        unsigned long yaddr_chan3_addr = yaddr_sub_chan_addr + ((isp->ch2_height*isp->ch2_width * 3) >> 1);
#else
	/*
	 * There is no channel 3 on this part, and its words are not spare: the
	 * EV300 indices below are EV200's statistics and 3D-NR-reference
	 * registers.
	 */
	isp_dbg("%s:id=%d, main_adr=%x, sub_addr=%x\n", __func__, id, yaddr_main_chan_addr, yaddr_sub_chan_addr);
#endif
	switch(id)
	{
    case BUFFER_ONE:
        //main chan
        isp->reg_blkaddr5[25] = yaddr_main_chan_addr;   //y1
        isp->reg_blkaddr5[26] = yaddr_main_chan_addr+isp->ch1_height*isp->ch1_width; //u1 
        isp->reg_blkaddr5[27] = yaddr_main_chan_addr+isp->ch1_height*isp->ch1_width*5/4 ;  //v1 
        //sub chan
        isp->reg_blkaddr5[28] = yaddr_sub_chan_addr;   //sub_y1
        isp->reg_blkaddr5[29] = yaddr_sub_chan_addr+isp->ch2_height*isp->ch2_width; //sub_u1 
        isp->reg_blkaddr5[30] = yaddr_sub_chan_addr+isp->ch2_height*isp->ch2_width*5/4 ;  //sub_v1 

#ifndef CONFIG_CPU_AK3918EV200
		isp->reg_blkaddr5[31] = yaddr_chan3_addr;
		isp->reg_blkaddr5[32] = yaddr_chan3_addr+(isp->ch2_height>>1)*(isp->ch2_width>>1); //sub_u1
        isp->reg_blkaddr5[33] = yaddr_chan3_addr+(isp->ch2_height>>1)*(isp->ch2_width>>1)*5/4;  //sub_v1
#endif
        break;
    case BUFFER_TWO:
        isp->reg_blkaddr1[31] = yaddr_main_chan_addr;   //y2
        isp->reg_blkaddr1[34] = yaddr_main_chan_addr+isp->ch1_height*isp->ch1_width; //u2 
        isp->reg_blkaddr1[37] = yaddr_main_chan_addr+isp->ch1_height*isp->ch1_width*5/4 ;  //v2  

        isp->reg_blkaddr1[40] = yaddr_sub_chan_addr;   //sy2
        isp->reg_blkaddr1[43] = yaddr_sub_chan_addr+isp->ch2_height*isp->ch2_width; //su2
        isp->reg_blkaddr1[46] = yaddr_sub_chan_addr+isp->ch2_height*isp->ch2_width*5/4 ;  //sv2

#ifndef CONFIG_CPU_AK3918EV200
		isp->reg_blkaddr1[49] = yaddr_chan3_addr;
		isp->reg_blkaddr1[52] = yaddr_chan3_addr+(isp->ch2_height>>1)*(isp->ch2_width>>1); //sub_u1
        isp->reg_blkaddr1[55] = yaddr_chan3_addr+(isp->ch2_height>>1)*(isp->ch2_width>>1)*5/4 ;  //sub_v1
#endif

        break;
    case BUFFER_THREE:
        isp->reg_blkaddr1[32] = yaddr_main_chan_addr;   //y3
        isp->reg_blkaddr1[35] = yaddr_main_chan_addr+isp->ch1_height*isp->ch1_width; //u3 
        isp->reg_blkaddr1[38] = yaddr_main_chan_addr+isp->ch1_height*isp->ch1_width*5/4 ;  //v3 

        isp->reg_blkaddr1[41] = yaddr_sub_chan_addr;   //sy3
        isp->reg_blkaddr1[44] = yaddr_sub_chan_addr+isp->ch2_height*isp->ch2_width; //su3 
        isp->reg_blkaddr1[47] = yaddr_sub_chan_addr+isp->ch2_height*isp->ch2_width*5/4 ;  //sv3

#ifndef CONFIG_CPU_AK3918EV200
		isp->reg_blkaddr1[50] = yaddr_chan3_addr;
		isp->reg_blkaddr1[53] = yaddr_chan3_addr+(isp->ch2_height>>1)*(isp->ch2_width>>1); //sub_u1
        isp->reg_blkaddr1[56] = yaddr_chan3_addr+(isp->ch2_height>>1)*(isp->ch2_width>>1)*5/4;  //sub_v1
#endif
        break;
    case BUFFER_FOUR:
        isp->reg_blkaddr1[33] = yaddr_main_chan_addr;   //y4
        isp->reg_blkaddr1[36] = yaddr_main_chan_addr+isp->ch1_height*isp->ch1_width; //u4 
        isp->reg_blkaddr1[39] = yaddr_main_chan_addr+isp->ch1_height*isp->ch1_width*5/4 ;  //v4

        isp->reg_blkaddr1[42] = yaddr_sub_chan_addr;   //sy4
        isp->reg_blkaddr1[45] = yaddr_sub_chan_addr+isp->ch2_height*isp->ch2_width; //su4
        isp->reg_blkaddr1[48] = yaddr_sub_chan_addr+isp->ch2_height*isp->ch2_width*5/4 ;  //sv4

#ifndef CONFIG_CPU_AK3918EV200
		isp->reg_blkaddr1[51] = yaddr_chan3_addr;
		isp->reg_blkaddr1[54] = yaddr_chan3_addr+(isp->ch2_height>>1)*(isp->ch2_width>>1); //sub_u1
        isp->reg_blkaddr1[57] = yaddr_chan3_addr+(isp->ch2_height>>1)*(isp->ch2_width>>1)*5/4;  //sub_v1
#endif
        break;
    default:
        break;
    }

  return 0;
}

int _isp_vo_set_stat_addr(enum buffer_id  id,unsigned long  stat_phy_addr, void *stat_virtual_addr)
{
	switch(id)
	{
	case BUFFER_ONE:
		isp->reg_blkaddr5[R5_STAT_ADDR1] = stat_phy_addr;    //y1
		isp->stat_addr[0] = (T_U32*)stat_virtual_addr;
		break;
	case BUFFER_TWO:
		isp->reg_blkaddr1[R1_STAT_ADDR2] = stat_phy_addr;   //y2
		isp->stat_addr[1] = (T_U32*)stat_virtual_addr;
		break;
	case BUFFER_THREE:
		isp->reg_blkaddr1[R1_STAT_ADDR2 + 1] = stat_phy_addr;   //y3
		isp->stat_addr[2] = (T_U32*)stat_virtual_addr;
		break;
	case BUFFER_FOUR:
		isp->reg_blkaddr1[R1_STAT_ADDR2 + 2] = stat_phy_addr;   //y4
		isp->stat_addr[3] = (T_U32*)stat_virtual_addr;
		break;
	default:
		break;
    }
    
    return 0;
}

int ak_isp_vo_get_using_frame_buf_id(void)
{
	int cmd, cnt,i;
	
	if(!isp)
		return 0;

	cmd = REG32(isp->base + ISP_ADDR_EN_ADDR); //lz0499 9_13
	cmd = (cmd>>ISP_ADDR_EN_SHIFT)&0x0f;
	cnt = 0;
	for(i=0; i<4; i++)
	{
		if(cmd&0x1)
			cnt++;
		cmd = cmd>>1;
	}
	
	if(cnt>1)
	{		
		if(isp->ae_drop_flag == 1)
		{
			isp->ae_drop_flag = 0;
			return isp->current_buffer|0x80;	//add flag, inform uplayer drop this frame
		}
		else if(isp->ae_drop_count>0)
		{
			unsigned long *stat_addr;
			unsigned long pixel_total = 0;
			int i;
			int curr_lumi;
			
			stat_addr = isp->stat_addr[isp->current_buffer];
       		for(i=0;i<256;i++)
			{
    			pixel_total += stat_addr[i];
			}


			if(pixel_total == 0)
			{ /* Statistics value sometimes unavailable; guard against divide-by-zero. */
				pixel_total = 1;
			}

			curr_lumi = stat_addr[256]/pixel_total;
			if(abs(isp->ae_last_lumi-curr_lumi)*5<isp->ae_last_lumi)
			{
				isp->ae_last_lumi = curr_lumi;
				return isp->current_buffer;
			}
			else
			{
				isp->ae_drop_flag = 0;
				//printk("dropone\n");
				return isp->current_buffer|0x80;	//add flag, inform uplayer drop this frame
			}
		}
		else
			return isp->current_buffer;
	}
	else
		return -1;
}

int ak_isp_vo_get_using_frame_buf_hwid(void)
{
	int frame_id;
	if(!isp)
		return 0;

	frame_id = REG32(isp->base + (STATUS_REG_ADDR-ISP_BASE_ADDR));
	frame_id = (frame_id>>MAIN_FRAME_BUF_ID)&0x03;
	isp->current_buffer = frame_id;

	return frame_id;
}

int ak_isp_is_continuous(void)
{
    enum isp_working_mode video_mode;
    
    if(!isp)
        return 0;

    video_mode = isp->cur_mode;
    if(video_mode == ISP_YUV_VIDEO_OUT
        ||video_mode == ISP_RGB_VIDEO_OUT)
        return 1;
    else
        return 0;
}

int _isp_set_pp_frame_ctrl(int ch1_frame_ctrl, int ch2_frame_ctrl,int ch3_frame_ctrl)//lz0499 7.21
{
	unsigned int cmd;
	
	cmd = 0;
	cmd = isp->reg_blkaddr4[R4_FRAME_CTRL];
#ifdef CONFIG_CPU_AK3918EV200
	CLEAR_BITS(cmd, 0, 16);
	cmd |= ((LOW_BITS(ch1_frame_ctrl, 8) << PP_FRAME1_CTRL)
		|  (LOW_BITS(ch2_frame_ctrl, 8) << PP_FRAME2_CTRL));
#else
	CLEAR_BITS(cmd, 0, 24);
	cmd |= ((LOW_BITS(ch1_frame_ctrl, 8) << PP_FRAME1_CTRL)
		|  (LOW_BITS(ch2_frame_ctrl, 8) << PP_FRAME2_CTRL)
		|  (LOW_BITS(ch3_frame_ctrl, 8) << PP_FRAME3_CTRL));// lz0499 9_5
#endif
	isp->reg_blkaddr4[R4_FRAME_CTRL] = cmd;
	
	return 0;
}

static int isp2_reg_init(void)
{
	AK_ISP_WB_GAIN wb_gain;
	unsigned long cmd;

	cmd = 0;
	cmd = isp->reg_blkaddr5[R5_ENABLE];
	CLEAR_BITS(cmd, SDB3_UPLOAD_EN, 1);
	CLEAR_BITS(cmd, SDB4_UPLOAD_EN, 1);
	CLEAR_BITS(cmd, SDB5_UPLOAD_EN, 1);

	SET_BITS(cmd, SDB3_UPLOAD_EN, 1);
	SET_BITS(cmd, SDB4_UPLOAD_EN, 1);
	SET_BITS(cmd, SDB5_UPLOAD_EN, 1);
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

	//general & timing setting
	_isp_set_misc_attr();
	ak_isp_vo_enable_irq_status(0x1);
	isp->reg_blkaddr2[51] = 0x40;   //RGB2YUV and Contrast Para
	_isp_set_pp_frame_ctrl(0xff,0xff,0x00); //lz0499 9_19 dfl disable ch3

	_isp_vo_set_stat_addr(0, isp->stat_handle, isp->stat_area);
	_isp_vo_set_stat_addr(1, isp->stat_handle, isp->stat_area);
	_isp_vo_set_stat_addr(2, isp->stat_handle, isp->stat_area);
	_isp_vo_set_stat_addr(3, isp->stat_handle, isp->stat_area);
    	
	//ae run info
	if(isp->exp_type_para.exp_type == 1) //AE default model:auto Exposure
   {
	   isp->ae_run_info_para.current_exp_time = isp->ae_para.exp_time_max;
		_cmos_updata_exp_time(isp->ae_run_info_para.current_exp_time);
		isp->ae_run_info_para.current_a_gain = isp->ae_para.a_gain_min;
		 _cmos_updata_a_gain(isp->ae_run_info_para.current_a_gain);
		isp->ae_run_info_para.current_d_gain = isp->ae_para.d_gain_min;
		_cmos_updata_d_gain(isp->ae_run_info_para.current_d_gain);
		isp->ae_run_info_para.current_isp_d_gain = isp->ae_para.isp_d_gain_min;
		_isp_update_isp_d_gain(isp->ae_run_info_para.current_isp_d_gain);
	}
	else
	{
		//default manual exposure: 256
		_cmos_updata_exp_time(isp->mae_para.exp_time);	
		isp->ae_run_info_para.current_exp_time = isp->mae_para.exp_time;
		_cmos_updata_a_gain(isp->mae_para.a_gain);
		isp->ae_run_info_para.current_a_gain = isp->mae_para.a_gain;
		_cmos_updata_d_gain(isp->mae_para.d_gain);
		isp->ae_run_info_para.current_d_gain = isp->mae_para.d_gain;
		_isp_update_isp_d_gain(isp->mae_para.isp_d_gain);
		isp->ae_run_info_para.current_isp_d_gain = isp->mae_para.isp_d_gain;
	}

	/* FIXME */
	_isp_para_change_with_envi(ENVI_OUTDOOR); 
	_set_nr1_attr(&(isp->nr1_para.linkage_nr1[ENVI_OUTDOOR]));
	_set_nr2_attr(&(isp->nr2_para.linkage_nr2[ENVI_OUTDOOR]));
	_set_sharp_attr(&(isp->sharp_para.linkage_sharp_attr[ENVI_OUTDOOR]));
	_set_ccm_attr(&(isp->ccm_para.ccm[ISP2_COLORTEMP_MODE_D65]));
	_isp_ccm_change_with_envi(ISP2_COLORTEMP_MODE_D65);
	_set_hue_attr(&(isp->hue_para.hue[ISP2_COLORTEMP_MODE_D65]));
	_isp_hue_change_with_envi(ISP2_COLORTEMP_MODE_D65);
		
	if(WB_OPS_TYPE_MANU==isp->wb_type_para.wb_type )
	{
	       wb_gain.r_gain = isp->mwb_para.r_gain;
	       wb_gain.g_gain = isp->mwb_para.g_gain;
	       wb_gain.b_gain = isp->mwb_para.b_gain;
	       wb_gain.r_offset = isp->mwb_para.r_offset;
	       wb_gain.g_offset = isp->mwb_para.g_offset;
	       wb_gain.b_offset = isp->mwb_para.b_offset;
	       ak_isp_updata_wb_gain(&wb_gain);
	}
	else
	{
		//isp->awb_algo.target_r_gain = isp->awb_algo.current_r_gain = wb_gain.r_gain = 255;
		//isp->awb_algo.target_g_gain = isp->awb_algo.current_g_gain = wb_gain.g_gain = 255;
		//isp->awb_algo.target_b_gain = isp->awb_algo.current_b_gain = wb_gain.b_gain = 255;
		isp->awb_stat_info_para.current_colortemp_index=ISP2_COLORTEMP_MODE_D65;
		isp->awb_needs_seed = 1;
		isp->awb_algo.current_r_gain = isp->awb_stat_info_para.r_gain =  isp->awb_algo.target_r_gain = isp->mwb_para.r_gain;
		isp->awb_algo.current_g_gain = isp->awb_stat_info_para.g_gain = isp->awb_algo.target_g_gain = isp->mwb_para.g_gain;
		isp->awb_algo.current_b_gain = isp->awb_stat_info_para.b_gain = isp->awb_algo.target_b_gain = isp->mwb_para.b_gain;
		
		wb_gain.r_gain = isp->mwb_para.r_gain;
	    wb_gain.g_gain = isp->mwb_para.g_gain;
	    wb_gain.b_gain = isp->mwb_para.b_gain;

		wb_gain.r_offset = 0;
		wb_gain.g_offset = 0;
		wb_gain.b_offset = 0;
		ak_isp_updata_wb_gain(&wb_gain);
		_set_awb_attr(&(isp->awb_para));
	}

	return 0;
}

int ak_isp_vi_start_capturing(void)
{
    unsigned long peri_status, cmd;
    isp_dbg("%s enter.\n", __func__);

    isp->current_buffer = -1;
    _isp_enable_buffer();
    cmd = REG32(isp->base + ISP_ADDR_EN_ADDR);//lz0499 9_13
	cmd = (cmd>>ISP_ADDR_EN_SHIFT)&0x0f;
	isp->next_buffer= 0;
#ifndef CONFIG_CPU_AK3918EV200
	cmd = low_4bits_revert((unsigned char )cmd);
#endif
	while(((1<<isp->next_buffer)&cmd)==0)
	{
		isp->next_buffer = (isp->next_buffer+1)%4;
	}

    //set up DMA table address
    REG32(isp->base+DMA_REG_ADDR) = isp->handle;
	cmd = REG32(isp->base+DMA_SDPC_ADDR);
	CLEAR_BITS(cmd, 0, 31);
	cmd |= LOW_BITS(isp->sdpc_handle, 30);
	REG32(isp->base+DMA_SDPC_ADDR) = cmd;

    isp2_reg_init();
    ak_isp_vo_update_regtable(MODE_ALL);
    //isp2_print_reg_table();
    
    /* start capturing bit */
    peri_status = REG32(isp->base + GEN_CTRL_ADDR); //add by xc
    SET_BITS(peri_status, 20, 1);
    REG32(isp->base + GEN_CTRL_ADDR) = peri_status;

    return 0;
}

int ak_isp_vi_stop_capturing(void)
{
    unsigned long peri_status;

    if (!isp)
        return -1;
    
    isp_dbg("%s enter.\n", __func__);
	int maxdelay = 200;
	while ((REG32(isp->base + 0x10) & (1 << 14))) {
		if (maxdelay > 0) {
			maxdelay--;
			isp->cb.cb_msleep(1);
		} else {
			isp->cb.cb_printk("[%s] reg10 b14 not clear\n", __func__);
			break;
		}
	}
	
    peri_status = REG32(isp->base + GEN_CTRL_ADDR); //add by xc
    CLEAR_BITS(peri_status, 20, 1);
    REG32(isp->base + GEN_CTRL_ADDR) = peri_status;

    return 0;   
}

/**
 * @brief: set wdr other param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_wdr_ex: wdr other param
 */
int _isp_set_wdr_ex_attr(AK_ISP_WDR_EX_ATTR *p_wdr_ex)
{
	unsigned long cmd = 0;
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->wdr_ex_para),p_wdr_ex, sizeof(AK_ISP_WDR_EX_ATTR));

	//yuv_reg_4
	cmd = 0;
	cmd = isp->reg_blkaddr3[3];
	CLEAR_BITS(cmd, 8, 24);
	cmd |= (LOW_BITS(p_wdr_ex->hdr_reverseW_g, 9) << HDR_REVERSEW_G) 
		|  (LOW_BITS(p_wdr_ex->hdr_reverseW_shift, 3) << HDR_REVERSEW_SHIFT)
		|  (LOW_BITS(p_wdr_ex->hdr_reverseH_g, 9) << HDR_REVERSEH_G)
		|  (LOW_BITS(p_wdr_ex->hdr_reverseH_shift, 3) << HDR_REVERSEH_SHIFT);
	isp->reg_blkaddr3[3] = cmd;
	 
	//yuv_reg_5
	cmd = 0;
	cmd = isp->reg_blkaddr3[4];
	CLEAR_BITS(cmd, 0, 12);
	cmd |= (LOW_BITS(p_wdr_ex->hdr_weight_g, 9) << HDR_WEIGHT_G) 
		|  (LOW_BITS(p_wdr_ex->hdr_weight_shift, 3) << HDR_WEIGHT_SHIFT);
	isp->reg_blkaddr3[4] = cmd;

	//yuv_reg_6
	cmd = 0;
	cmd = isp->reg_blkaddr3[5];
	CLEAR_BITS(cmd, HDR_WEIGHT_K, 4);
	cmd |= (LOW_BITS(p_wdr_ex->hdr_weight_k, 4) << HDR_WEIGHT_K);
	isp->reg_blkaddr3[5] = cmd;
	
	ak_isp_vo_update_setting();
	
    return 0;
}

int ak_isp_vi_set_crop(int sx, int sy, int width, int height)
{
    unsigned long cmd = 0;
    AK_ISP_WDR_EX_ATTR wdr_ex;
    int tnr_revW, tnr_revH;
    int i, g;
	int height_block_size=0;
#ifndef CONFIG_CPU_AK3918EV200
	int height_block_num_flg=0;
#endif
    
    isp_dbg("%s enter. (%d,%d,%d,%d)\n", __func__,  sx, sy, width,  height);

    //cut_reg_1
    cmd = (LOW_BITS(sx, 13) << CUT_STR_XPOS) 
    	| (LOW_BITS(sy, 13) << CUT_STR_YPOS);
    //*pReg++ = cmd;
    isp->reg_blkaddr1[0] = cmd;

    //cut_reg_2
    cmd = 0;
    cmd = isp->reg_blkaddr1[1];
    CLEAR_BITS(cmd, 0, 28);
    cmd = (LOW_BITS(width, 13) << CUT_WIN_WIDTH)
    	| (LOW_BITS(height, 13) << CUT_WIN_HEIGHT);
    //(IspC->sub_sample << DOWNSAMPLE);
    //*pReg++ = cmd;
    isp->reg_blkaddr1[1] = cmd;

    isp->cut_width = width;
    isp->cut_height= height;

    wdr_ex.hdr_blkW= width/16;
    wdr_ex.hdr_blkH = height/8;
    for(i=0; i<7; i++)
    {
		g = (1<<(i+1+8))/wdr_ex.hdr_blkW;
		if(g>511)
			break;
    }
    wdr_ex.hdr_reverseW_shift = i;
    wdr_ex.hdr_reverseW_g = (1<<(i+8))/wdr_ex.hdr_blkW;
    for(i=0; i<7; i++)
    {
		g = (1<<(i+1+8))/wdr_ex.hdr_blkH;
		if(g>511)
			break;
    }
    wdr_ex.hdr_reverseH_shift = i;
    wdr_ex.hdr_reverseH_g = (1<<(i+8))/wdr_ex.hdr_blkH;
	
	for(i=0; i<7; i++)
    {
		g = (wdr_ex.hdr_blkW*wdr_ex.hdr_blkH)>>i;
		if(g<256)
			break;
    }
    wdr_ex.hdr_weight_shift= i;
    wdr_ex.hdr_weight_g= 65536/((wdr_ex.hdr_blkW*wdr_ex.hdr_blkH)>>i);
	
	wdr_ex.hdr_weight_g = OV_MIN(wdr_ex.hdr_weight_g, 511);
    wdr_ex.hdr_weight_k = 8;
    _isp_set_wdr_ex_attr(&wdr_ex);

	//set tnr motion stat max & revW & revH
	//lz0499 9_12
#ifdef CONFIG_CPU_AK3918EV200
	/*
	 * This part has no selectable TNR block height: REG3[20] bits 24-30
	 * (TNR_HEIGHT_BLOCK_SIZE) and bit 31 (TNR_HEIGHT_BLOCK_NUM) do not
	 * exist, its set_crop never writes REG3[20], and its block size is
	 * always height/16.
	 *
	 * Keeping EV300's %24 branch would be silently wrong at exactly the
	 * capture resolution: 720 % 24 == 0 gives 4096/30 = 136 where
	 * the hardware expects 4096/45 = 91, mis-scaling the 3D-NR motion
	 * field.
	 */
	height_block_size = height/16;
	isp_info("ISP: EV200 tnr block %d (h=%d, /16)\n", height_block_size, height);
#else
	if(height%24 == 0)
		{
			height_block_num_flg = TNR_HEIGHT_BLOCKSIZE_24;
			height_block_size = height/24;
		}
	else
		{
			height_block_num_flg = TNR_HEIGHT_BLOCKSIZE_16;
			height_block_size = height/16;
		}
	cmd  = 0;
	cmd = isp->reg_blkaddr3[20];
	CLEAR_BITS(cmd, 24, 8);
	cmd |= (LOW_BITS(height_block_size,7) << TNR_HEIGHT_BLOCK_SIZE)
		|  (LOW_BITS(height_block_num_flg,1) << TNR_HEIGHT_BLOCK_NUM);
	isp->reg_blkaddr3[20] = cmd;
#endif


    tnr_revW = 4096/(width/32);
    tnr_revH = 4096/(height_block_size);
    cmd = 0;
    cmd = (0xffff) 
    	| (LOW_BITS(tnr_revW, 8) << TNR_REVW)
    	| (LOW_BITS(tnr_revH, 8) << TNR_REVH);
    isp->reg_blkaddr3[21] = cmd;

    return 0;
}

int ak_isp_vi_apply_mode(enum isp_working_mode mode)
{
    unsigned long peri_status;
    
    isp_dbg("%s enter. %d\n", __func__, mode);

    isp->cur_mode = mode;
    switch(isp->cur_mode) 
    {
    case ISP_RGB_VIDEO_OUT:
        peri_status = REG32(isp->base + GEN_CTRL_ADDR); //add by xc
	    CLEAR_BITS(peri_status, DATA_FORMAT, 2);
        CLEAR_BITS(peri_status, FORCE_STOP, 1);
        SET_BITS(peri_status, 	VIDEO_MODE, 1);
        CLEAR_BITS(peri_status, GO_YUV, 1);
        CLEAR_BITS(peri_status, GO_UPLOAD, 1);
        REG32(isp->base + GEN_CTRL_ADDR) = peri_status;
        break;
        
    case ISP_RGB_OUT:
        peri_status = REG32(isp->base + GEN_CTRL_ADDR);
        CLEAR_BITS(peri_status, DATA_FORMAT, 2);
        CLEAR_BITS(peri_status, FORCE_STOP, 1);
        CLEAR_BITS(peri_status, VIDEO_MODE, 1);
        CLEAR_BITS(peri_status, GO_YUV, 1);
        CLEAR_BITS(peri_status, GO_UPLOAD, 1);
        REG32(isp->base + GEN_CTRL_ADDR) = peri_status;
        break;
        
    case ISP_YUV_OUT:
        peri_status = REG32(isp->base + GEN_CTRL_ADDR);
        CLEAR_BITS(peri_status, DATA_FORMAT, 2);
    	SET_BITS(peri_status, 	DATA_FORMAT, 1);
        CLEAR_BITS(peri_status, FORCE_STOP, 1);
        CLEAR_BITS(peri_status, VIDEO_MODE, 1);
        SET_BITS(peri_status, 	GO_YUV, 1);
        CLEAR_BITS(peri_status, GO_UPLOAD, 1);
        REG32(isp->base + GEN_CTRL_ADDR) = peri_status;
		ak_isp_vi_set_sub128(1);
        break;
        
    case ISP_YUV_VIDEO_OUT:
        peri_status = REG32(isp->base + GEN_CTRL_ADDR);
        CLEAR_BITS(peri_status, DATA_FORMAT, 2);
    	SET_BITS(peri_status, 	DATA_FORMAT, 1);
        CLEAR_BITS(peri_status, FORCE_STOP, 1);
        SET_BITS(peri_status, 	VIDEO_MODE, 1);
        SET_BITS(peri_status, 	GO_YUV, 1);
        CLEAR_BITS(peri_status, GO_UPLOAD, 1);
        REG32(isp->base + GEN_CTRL_ADDR) = peri_status;
		ak_isp_vi_set_sub128(1);
        break;
    
    case ISP_JPEG_MODE:
        peri_status = REG32(isp->base + GEN_CTRL_ADDR);
        CLEAR_BITS(peri_status, DATA_FORMAT, 2);
    	SET_BITS(peri_status, 	JPEG_DATA, 1);
       	CLEAR_BITS(peri_status, FORCE_STOP, 1);
        CLEAR_BITS(peri_status, VIDEO_MODE, 1);
        SET_BITS(peri_status, 	GO_UPLOAD, 1);
        REG32(isp->base + GEN_CTRL_ADDR) = peri_status;
        break;  
        
	default:
        //REG32(isp->base + GEN_CTRL_ADDR) |= (3 << 30);
        printk("error isp work mode\n");
    }

    return 0;
}

int ak_isp_vo_set_main_channel_scale(int width, int height)
{
    unsigned int cmd = 0;
    unsigned int sample;
    
    isp_dbg("%s enter.(%d,%d,%d,%d,%d)\n", __func__, width, height, isp->sub_sample, isp->cut_width, isp->cut_height);
    sample = 0x1 << (T_U8)isp->sub_sample;

    isp->ch1_width = width;
    isp->ch1_height = height;
    //int iWidth = ((isp->cut_width/2)/sample + (((isp->cut_width/2)%sample)?1:0))*2;
    int iHeight = ((isp->cut_height/2)/sample + (((isp->cut_height/2)%sample)?1:0))*2;

    if((isp->ch1_width-1)>2*(width-1))
    {
        isp->chl1_xrate  = 65536/isp->ch1_width;
    }
    else
    {
        isp->chl1_xrate  = 65536/(isp->ch1_width -1);
    }
    if((isp->ch1_height-1)>2*(iHeight-1))
    {
        isp->chl1_yrate  = 65536/isp->ch1_height;
    }
    else
    {
        isp->chl1_yrate  = 65536/(isp->ch1_height -1);
    }

    //pp_reg_31
    cmd = 0;
    cmd = (LOW_BITS(isp->chl1_xrate, 12) << PP_WIDTH1_PARA) 
    	| (LOW_BITS(isp->chl1_yrate, 12) << PP_HEIGHT1_PARA);
    //*pReg++ = cmd;
    isp->reg_blkaddr4[R4_MAIN_SCALE] = cmd;

    //pp_reg_49
    cmd = 0;
    cmd = (LOW_BITS(isp->ch1_width, PP_DST_BITS) << PP_DST1_WIDTH)
    	| (LOW_BITS(isp->ch1_height, PP_DST_BITS) << pp_DST1_HEIGHT);
    //*pReg++ = cmd;
    isp->reg_blkaddr4[R4_MAIN_SCALE + 1] = cmd;
    
    return 0;   
}

int ak_isp_vo_get_main_channel_scale(int *width, int *height)
{
    isp_dbg("%s enter.\n", __func__);  
    *width = isp->ch1_width;
    *height = isp->ch1_height;
    
    return 0;   
}

int ak_isp_vo_set_sub_channel_scale(int width, int height)
{
    unsigned int cmd = 0;
    
    isp_dbg("%s enter.\n", __func__);
    isp->ch2_width = width;
    isp->ch2_height = height;
	//unsigned int sample = 0x1 << (T_U8)isp->sub_sample;
    //int iWidth = ((isp->cut_width/2)/sample + (((isp->cut_width/2)%sample)?1:0))*2;
    //int iHeight = ((isp->cut_height/2)/sample + (((isp->cut_height/2)%sample)?1:0))*2;

    isp->chl2_yrate  = (65536+isp->ch2_height /2)/(isp->ch2_height -1);
    isp->chl2_xrate = 65536/(isp->ch2_width-1);

    //pp_reg_33
    cmd = 0;
    cmd = (LOW_BITS(isp->chl2_xrate, 12) << PP_WIDTH2_PARA) 
    	| (LOW_BITS(isp->chl2_yrate, 12) << PP_HEIGHT2_PARA);
    //*pReg++ = cmd;
    isp->reg_blkaddr4[R4_SUB_SCALE] = cmd;

    //pp_reg_51
    cmd = 0;
    cmd = (LOW_BITS(isp->ch2_width, PP_DST_BITS) << PP_DST2_WIDTH)
    	| (LOW_BITS(isp->ch2_height, PP_DST_BITS) << pp_DST2_HEIGHT);
    isp->reg_blkaddr4[R4_SUB_SCALE + 1] = cmd;

    return 0;
}

int ak_isp_vo_get_sub_channel_scale(int *width, int *height)
{
    isp_dbg("%s enter.\n", __func__);
     *width = isp->ch2_width;
    *height = isp->ch2_height;

    return 0;
}

/**
 * @brief: set blc param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_blc:  blc param
 */
/************************************************************************************************/
int ak_isp_vp_set_blc_attr(AK_ISP_BLC_ATTR *p_blc)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->blc_para),p_blc,sizeof(AK_ISP_BLC_ATTR));
	//ak_isp_vo_update_setting();
	isp->linkage_para_update_flag =1;

    return 0;
}

/**
 * @brief: get blc param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_blc:  blc param
 */

int ak_isp_vp_get_blc_attr(AK_ISP_BLC_ATTR *p_blc)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_blc,&(isp->blc_para),sizeof(AK_ISP_BLC_ATTR));

   	return 0;
}

/**
 * @brief: set lsc param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_lsc:  lsc param
 */
int _set_lsc_attr(AK_ISP_LSC_ATTR *p_lsc)
{
	T_U32 cmd;
    isp_dbg("%s enter.\n", __func__);
    
    ////lenc_reg_1
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_b[0], 8) << LENS_COEF0_BB_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_b[0], 8) << LENS_COEF0_BB_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_b[0], 8) << LENS_COEF0_BB_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_b[0], 8) << LENS_COEF0_BB_B);
    isp->reg_blkaddr1[2] = cmd;

    //lenc_reg_2
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_b[1], 8) << LENS_COEF1_BB_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_b[1], 8) << LENS_COEF1_BB_GR)
        | (LOW_BITS(p_lsc->lsc_gb_coef.coef_b[1], 8) << LENS_COEF1_BB_GB)
        | (LOW_BITS(p_lsc->lsc_b_coef.coef_b[1], 8) << LENS_COEF1_BB_B);
    isp->reg_blkaddr1[3] = cmd;

    //lenc_reg_3
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_b[2], 8) << LENS_COEF2_BB_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_b[2], 8) << LENS_COEF2_BB_GR) 
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_b[2], 8) << LENS_COEF2_BB_GB) 
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_b[2], 8) << LENS_COEF2_BB_B);
    
    isp->reg_blkaddr1[4] = cmd;

    //lenc_reg_4
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_b[3], 8) << LENS_COEF3_BB_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_b[3], 8) << LENS_COEF3_BB_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_b[3], 8) << LENS_COEF3_BB_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_b[3], 8) << LENS_COEF3_BB_B);
   	isp->reg_blkaddr1[5] = cmd;

   	//lenc_reg_5
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_b[4], 8) << LENS_COEF4_BB_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_b[4], 8) << LENS_COEF4_BB_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_b[4], 8) << LENS_COEF4_BB_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_b[4], 8) << LENS_COEF4_BB_B);
   	isp->reg_blkaddr1[6] = cmd;

   	//lenc_reg_6
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_b[5], 8) << LENS_COEF5_BB_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_b[5], 8) << LENS_COEF5_BB_GR)
        | (LOW_BITS(p_lsc->lsc_gb_coef.coef_b[5], 8) << LENS_COEF5_BB_GB)
        | (LOW_BITS(p_lsc->lsc_b_coef.coef_b[5], 8) << LENS_COEF5_BB_B);
    isp->reg_blkaddr1[7] = cmd;

    //lenc_reg_7
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_b[6], 8) << LENS_COEF6_BB_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_b[6], 8) << LENS_COEF6_BB_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_b[6], 8) << LENS_COEF6_BB_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_b[6], 8) << LENS_COEF6_BB_B);
    isp->reg_blkaddr1[8] = cmd;

    //lenc_reg_8
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_b[7], 8) << LENS_COEF7_BB_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_b[7], 8) << LENS_COEF7_BB_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_b[7], 8) << LENS_COEF7_BB_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_b[7], 8) << LENS_COEF7_BB_B);
    isp->reg_blkaddr1[9] = cmd;

    //lenc_reg_9
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_b[8], 8) << LENS_COEF8_BB_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_b[8], 8) << LENS_COEF8_BB_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_b[8], 8) << LENS_COEF8_BB_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_b[8], 8) << LENS_COEF8_BB_B);
    isp->reg_blkaddr1[10] =cmd;

    //lenc_reg_10
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_b[9], 8) << LENS_COEF9_BB_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_b[9], 8) << LENS_COEF9_BB_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_b[9], 8) << LENS_COEF9_BB_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_b[9], 8) << LENS_COEF9_BB_B);
    isp->reg_blkaddr1[11] =cmd;

    //lenc_reg_11
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_c[0], 10) << LENS_COEF0_CC_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_c[0], 10) << LENS_COEF0_CC_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_c[0], 10) << LENS_COEF0_CC_GB);
    isp->reg_blkaddr1[12] =cmd;

    //lenc_reg_12
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_b_coef.coef_c[0], 10) << LENS_COEF0_CC_B)
    	| (LOW_BITS(p_lsc->lsc_r_coef.coef_c[1], 10)<< LENS_COEF1_CC_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_c[1], 10)<< LENS_COEF1_CC_GR);
    isp->reg_blkaddr1[13] =cmd;

    //lenc_reg_13
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_gb_coef.coef_c[1], 10) << LENS_COEF1_CC_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_c[1], 10) << LENS_COEF1_CC_B)
    	| (LOW_BITS(p_lsc->lsc_r_coef.coef_c[2], 10)<< LENS_COEF2_CC_R);
    isp->reg_blkaddr1[14] =cmd;
    

    //lenc_reg_14
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_gr_coef.coef_c[2], 10)<< LENS_COEF2_CC_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_c[2], 10)<< LENS_COEF2_CC_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_c[2], 10) << LENS_COEF2_CC_B);
    isp->reg_blkaddr1[15] =cmd;

    //lenc_reg_15
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_c[3], 10) << LENS_COEF3_CC_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_c[3], 10)<< LENS_COEF3_CC_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_c[3], 10)<< LENS_COEF3_CC_GB);
    isp->reg_blkaddr1[16] =cmd;

    //lenc_reg_16
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_b_coef.coef_c[3], 10) << LENS_COEF3_CC_B)
    	| (LOW_BITS(p_lsc->lsc_r_coef.coef_c[4], 10)<< LENS_COEF4_CC_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_c[4], 10)<< LENS_COEF4_CC_GR);
    isp->reg_blkaddr1[17] =cmd;

    //lenc_reg_17
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_gb_coef.coef_c[4] , 10)<< LENS_COEF4_CC_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_c[4], 10) << LENS_COEF4_CC_B)
    	| (LOW_BITS(p_lsc->lsc_r_coef.coef_c[5], 10) << LENS_COEF5_CC_R);
    isp->reg_blkaddr1[18] =cmd;

    //lenc_reg_18
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_gr_coef.coef_c[5], 10)<< LENS_COEF5_CC_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_c[5], 10)<< LENS_COEF5_CC_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_c[5], 10)<< LENS_COEF5_CC_B);
    isp->reg_blkaddr1[19] =cmd;

    //lenc_reg_19
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_c[6], 10)<< LENS_COEF6_CC_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_c[6], 10)<< LENS_COEF6_CC_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_c[6], 10) << LENS_COEF6_CC_GB);
    isp->reg_blkaddr1[20] =cmd;

    //lenc_reg_20
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_b_coef.coef_c[6], 10)<< LENS_COEF6_CC_B)
    	| (LOW_BITS(p_lsc->lsc_r_coef.coef_c[7], 10)<< LENS_COEF7_CC_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_c[7], 10)<< LENS_COEF7_CC_GR);
    isp->reg_blkaddr1[21] =cmd;

    //lenc_reg_21
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_gb_coef.coef_c[7], 10) << LENS_COEF7_CC_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_c[7], 10)<< LENS_COEF7_CC_B)
    	| (LOW_BITS(p_lsc->lsc_r_coef.coef_c[8], 10)<< LENS_COEF8_CC_R);
    isp->reg_blkaddr1[22] =cmd;

    //lenc_reg_22
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_gr_coef.coef_c[8], 10) << LENS_COEF8_CC_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_c[8], 10) << LENS_COEF8_CC_GB)
    	| (LOW_BITS(p_lsc->lsc_b_coef.coef_c[8], 10)<< LENS_COEF8_CC_B);
    isp->reg_blkaddr1[23] =cmd;

    //lenc_reg_23
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_r_coef.coef_c[9], 10)<< LENS_COEF9_CC_R)
    	| (LOW_BITS(p_lsc->lsc_gr_coef.coef_c[9], 10)<< LENS_COEF9_CC_GR)
    	| (LOW_BITS(p_lsc->lsc_gb_coef.coef_c[9], 10)<< LENS_COEF9_CC_GB);
    isp->reg_blkaddr1[24] =cmd;

    //lenc_reg_24
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->lsc_b_coef.coef_c[9], 10)<< LENS_COEF9_CC_B)
    	| (LOW_BITS(p_lsc->range[0], 10) << LENS_RANGE0_RR)
    	| (LOW_BITS(p_lsc->range[1], 10) << LENS_RANGE1_RR);
    isp->reg_blkaddr1[25] =cmd;

    //lenc_reg_25
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->range[2], 10)  << LENS_RANGE2_RR)
    	| (LOW_BITS(p_lsc->range[3], 10) << LENS_RANGE3_RR)
    	| (LOW_BITS(p_lsc->range[4], 10) << LENS_RANGE4_RR);
    isp->reg_blkaddr1[26] =cmd;
    

    //lenc_reg_26
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->range[5], 10) << LENS_RANGE5_RR)
    	| (LOW_BITS(p_lsc->range[6], 10) << LENS_RANGE6_RR)
    	| (LOW_BITS(p_lsc->range[7], 10) << LENS_RANGE7_RR);
    isp->reg_blkaddr1[27] =cmd;

    //lenc_reg_27
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->range[8], 10) << LENS_RANGE8_RR)
    	| (LOW_BITS(p_lsc->range[9], 10) << LENS_RANGE9_RR);
    isp->reg_blkaddr1[28] =cmd;

    //lenc_reg_28
    cmd = 0;
    cmd = (LOW_BITS(p_lsc->xref, 12) << LENS_XREF)
    	| (LOW_BITS(p_lsc->yref, 12) << LENS_YREF);
    isp->reg_blkaddr1[29] =cmd;
    

    //lenc_reg_29
    cmd = 0;
    cmd = (LOW_BITS((p_lsc->xref*p_lsc->xref + p_lsc->yref*p_lsc->yref), 23) << LENS_1STSQR)
    	| (LOW_BITS(p_lsc->lsc_shift, 4) << LENS_LSC_SHIFT);
    isp->reg_blkaddr1[30] =cmd;

   	//enable
    cmd = 0;
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd , LEN_ADJUST_EN, 1);
    cmd |= (LOW_BITS(p_lsc->enable, 1) << LEN_ADJUST_EN);
    isp->reg_blkaddr5[R5_ENABLE] = cmd;
    
	ak_isp_vo_update_setting();
                    
    return 0;
}

int ak_isp_vp_set_lsc_attr(AK_ISP_LSC_ATTR *p_lsc)
{
    isp_dbg("%s enter.\n", __func__);
    
    isp->cb.cb_memcpy(&(isp->lsc_para),p_lsc,sizeof(AK_ISP_LSC_ATTR));
    isp->linkage_para_update_flag = 1;
                    
    return 0;
}

/**
 * @brief: get lsc param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_lsc:  lsc param
 */
 int ak_isp_vp_get_lsc_attr( AK_ISP_LSC_ATTR *p_lsc)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_lsc,&(isp->lsc_para),sizeof(AK_ISP_LSC_ATTR));

    return 0;
}

/**
 * @brief: set rgb gamma param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_rgb_gamma:  rgb gamma param
 */
int ak_isp_vp_set_rgb_gamma_attr(AK_ISP_RGB_GAMMA_ATTR *p_rgb_gamma)
{
    int i=0,j=0;
    unsigned long cmd = 0;
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->rgb_gamma_para),p_rgb_gamma,sizeof(AK_ISP_RGB_GAMMA_ATTR));

    for (i=1; i<129; i++)
    {
        cmd = 0;
        cmd = (p_rgb_gamma->r_gamma[i]) |
              (p_rgb_gamma->g_gamma[i] << 10) |
              (p_rgb_gamma->b_gamma[i] << 20);
        j = i-1;
        isp->cfg_mem_blkaddr2[j] = cmd;
        j++;
    }

	ak_isp_vo_update_setting();
	
    return 0;
}

/**
 * @brief: get rgb gamma param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_rgb_gamma:  rgb gamma param
 */
int ak_isp_vp_get_rgb_gamma_attr(AK_ISP_RGB_GAMMA_ATTR *p_rgb_gamma)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_rgb_gamma,&(isp->rgb_gamma_para),sizeof(AK_ISP_RGB_GAMMA_ATTR));

    return 0;
}

/**
 * @brief: set y gamma param
 * @author: lz
 * @date: 2016-9-9
 * @param [in] *p_y_gamma:  y gamma param
 */
int ak_isp_vp_set_y_gamma_attr(AK_ISP_Y_GAMMA_ATTR *p_y_gamma)
{
	int i=0,j=0;
	unsigned long cmd = 0;
	isp_dbg("%s enter.\n", __func__);
	memcpy(&(isp->y_gamma_para),p_y_gamma,sizeof(AK_ISP_Y_GAMMA_ATTR));

	for (i=1; i<127; i+=3)
	{
		cmd = 0;
		cmd = (p_y_gamma->ygamma[i]) |
			  (p_y_gamma->ygamma[i+1] << 10) |
			  (p_y_gamma->ygamma[i+2] << 20);
		isp->cfg_mem_blkaddr3[j] = cmd;
		j++;
	}

	cmd = 0;
	cmd = (p_y_gamma->ygamma[127]) |
		  (p_y_gamma->ygamma[128] << 10);
	isp->cfg_mem_blkaddr3[42] = cmd;

	//yuv_reg_1
	cmd = 0;
	cmd = isp->reg_blkaddr3[0];
    CLEAR_BITS(cmd, 0, 30);
	cmd |= (LOW_BITS(p_y_gamma->ygamma_cnoise_yth1, 10) << YGAMMA_CNOISE_TH1)
		|  (LOW_BITS(p_y_gamma->ygamma_cnoise_yth2, 10) << YGAMMA_CNOISE_TH2)
		|  (LOW_BITS(p_y_gamma->ygamma_cnoise_gain, 10) << YGAMMA_CNOISE_GAIN);
	isp->reg_blkaddr3[0] = cmd;

	//yuv_reg_2
	cmd = 0;
	cmd = isp->reg_blkaddr3[1];
	CLEAR_BITS(cmd, 0, 13);
	cmd |= (LOW_BITS(p_y_gamma->ygamma_cnoise_slop, 8) << YGAMMA_CNOISE_SLOP)
		|  (LOW_BITS(p_y_gamma->ygamma_uv_adjust_level, 5) << YGAMMA_UV_ADJUST_LEVEL);
	isp->reg_blkaddr3[1] = cmd;

	cmd = 0;											
	cmd = isp->reg_blkaddr5[R5_ENABLE];
	CLEAR_BITS(cmd, YGAMMA_UV_ADJUST_EN, 1);
	cmd |= (LOW_BITS(p_y_gamma->ygamma_uv_adjust_enable, 1) << YGAMMA_UV_ADJUST_EN);
	isp->reg_blkaddr5[R5_ENABLE] = cmd;

	ak_isp_vo_update_setting();
	
	return 0;
}

/**
 * @brief: get y gamma param
 * @author: lz
 * @date: 2016-9-9
 * @param [out] *p_y_gamma:  y gamma param
 */
int ak_isp_vp_get_y_gamma_attr(AK_ISP_Y_GAMMA_ATTR *p_y_gamma)
{
	isp_dbg("%s enter.\n", __func__);
	memcpy(p_y_gamma,&(isp->y_gamma_para),sizeof(AK_ISP_Y_GAMMA_ATTR));

	return 0;
}

/**
 * @brief: set raw gamma param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_raw_lut:  raw gamma param
 */
int ak_isp_vp_set_raw_lut_attr(AK_ISP_RAW_LUT_ATTR *p_raw_lut)
{
    int i = 0,j=0;
    unsigned long cmd = 0;
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->raw_lut_para),p_raw_lut,sizeof(AK_ISP_RAW_LUT_ATTR));

    for (i=1; i<129; i++)
    {
        cmd = 0;
        cmd = (p_raw_lut->raw_r[i]) |
            (p_raw_lut->raw_g[i] << 10) |
            (p_raw_lut->raw_b[i] << 20);
        j = i-1;
        isp->cfg_mem_blkaddr1[j] = cmd;
        j++;
    }

	ak_isp_vo_update_setting();
	
    return 0;
}

/**
 * @brief: get raw gamma param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_raw_lut:  raw gamma param
 */
int ak_isp_vp_get_raw_lut_attr(AK_ISP_RAW_LUT_ATTR *p_raw_lut)
{
    isp_dbg("%s enter.\n", __func__); 
    memcpy(p_raw_lut,&(isp->raw_lut_para),sizeof(AK_ISP_RAW_LUT_ATTR));

    return 0;
}

/**
 * @brief: set envi dpc param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_dpc:  dpc param
 */
int _set_dpc_attr(AK_ISP_DDPC* p_dpc)
{
    unsigned long cmd = 0;  
    isp_dbg("%s enter.\n", __func__);
    
    //raw_reg_20
    cmd = 0;
	cmd = isp->reg_blkaddr2[19];
	CLEAR_BITS(cmd, 0, 10);
	CLEAR_BITS(cmd, 16, 2);
    cmd |= (LOW_BITS(p_dpc->ddpc_th, 10) << DPC_TH)
    	|  (LOW_BITS(p_dpc->black_dpc_enable, 1) << BLACK_DPC_EN)
    	|  (LOW_BITS(p_dpc->white_dpc_enable, 1) << WHITE_DPC_EN);
    isp->reg_blkaddr2[19] = cmd;
    
	//reg_block5  enable_reg
    cmd = 0;
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, DDPC_EN, 1);
    cmd |= (LOW_BITS(p_dpc->ddpc_enable, 1) << DDPC_EN);
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

	ak_isp_vo_update_setting();

    return 0;
} 

/**
 * @brief: set dpc param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_dpc:  dpc param
 */
int ak_isp_vp_set_dpc_attr(AK_ISP_DDPC_ATTR *p_dpc)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->dpc_para), p_dpc,sizeof(AK_ISP_DDPC_ATTR));
    isp->linkage_para_update_flag =  1;

    return 0;
}

/**
 * @brief: get dpc param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_dpc:  dpc param
 */
int ak_isp_vp_get_dpc_attr(AK_ISP_DDPC_ATTR *p_dpc)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_dpc,&(isp->dpc_para),sizeof(AK_ISP_DDPC_ATTR));

    return 0;
}

/**
 * @brief: set sdpc param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_sdpc:  sdpc param
 */
int ak_isp_vp_set_sdpc_attr(AK_ISP_SDPC_ATTR *p_sdpc)
{
    int sdpc_cfg;
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->sdpc_para),p_sdpc,sizeof(AK_ISP_SDPC_ATTR));
    memcpy(isp->sdpc_area, isp->sdpc_para.sdpc_table, 1024*4);

	sdpc_cfg = REG32(isp->base + DMA_SDPC_ADDR);
    if(isp->sdpc_para.sdpc_enable)
		SET_BITS(sdpc_cfg, 31, 1);
    else
        CLEAR_BITS(sdpc_cfg, 31, 1);

    REG32(isp->base + DMA_SDPC_ADDR) = sdpc_cfg;

	ak_isp_vo_update_setting();
        return 0;
}

/**
 * @brief: get sdpc param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_sdpc:  sdpc param
 */
int ak_isp_vp_get_sdpc_attr(AK_ISP_SDPC_ATTR *p_sdpc)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_sdpc,&(isp->sdpc_para),sizeof(AK_ISP_SDPC_ATTR));

    return 0;
}

/**
 * @brief: set raw noise remove param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *nr1:  raw noise remove  param
 */
int _set_nr1_attr(AK_ISP_NR1 *nr1)
{
    unsigned long cmd = 0;
    isp_dbg("%s enter.\n", __func__);

    //raw_reg_21
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_rtbl[0], 10) << NR_WEIGHT_TBL_R0)
    	| (LOW_BITS(nr1->nr1_weight_rtbl[1], 10) << NR_WEIGHT_TBL_R1)
    	| (LOW_BITS(nr1->nr1_weight_rtbl[2], 10) << NR_WEIGHT_TBL_R2);
    //*pReg++ = cmd;
    isp->reg_blkaddr2[20] = cmd;

    //raw_reg_22
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_rtbl[3], 10) << NR_WEIGHT_TBL_R3)
    	| (LOW_BITS(nr1->nr1_weight_rtbl[4], 10) << NR_WEIGHT_TBL_R4)
    	| (LOW_BITS(nr1->nr1_weight_rtbl[5], 10) << NR_WEIGHT_TBL_R5);
    isp->reg_blkaddr2[21] = cmd;
    
    //raw_reg_23
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_rtbl[6], 10) << NR_WEIGHT_TBL_R6)
    	| (LOW_BITS(nr1->nr1_weight_rtbl[7], 10) << NR_WEIGHT_TBL_R7) 
    	| (LOW_BITS(nr1->nr1_weight_rtbl[8], 10) << NR_WEIGHT_TBL_R8);
    isp->reg_blkaddr2[22] = cmd;

    //raw_reg_24
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_rtbl[9], 10) << NR_WEIGHT_TBL_R9)
    	| (LOW_BITS(nr1->nr1_weight_rtbl[10], 10) << NR_WEIGHT_TBL_R10)
    	| (LOW_BITS(nr1->nr1_weight_rtbl[11], 10) << NR_WEIGHT_TBL_R11);
    isp->reg_blkaddr2[23] = cmd;
    //raw_reg_25
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_rtbl[12], 10) << NR_WEIGHT_TBL_R12)
    	| (LOW_BITS(nr1->nr1_weight_rtbl[13], 10) << NR_WEIGHT_TBL_R13)
    	| (LOW_BITS(nr1->nr1_weight_rtbl[14], 10) << NR_WEIGHT_TBL_R14);
    isp->reg_blkaddr2[24] = cmd;

    //raw_reg_26
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_rtbl[15], 10) << NR_WEIGHT_TBL_R15)
    	| (LOW_BITS(nr1->nr1_weight_rtbl[16], 10) << NR_WEIGHT_TBL_R16)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[0], 10) << NR_WEIGHT_TBL_G0);
    isp->reg_blkaddr2[25] = cmd;

    //raw_reg_27
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_gtbl[1], 10) << NR_WEIGHT_TBL_G1)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[2], 10) << NR_WEIGHT_TBL_G2)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[3], 10) << NR_WEIGHT_TBL_G3);
    isp->reg_blkaddr2[26] = cmd;

    //raw_reg_28
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_gtbl[4], 10) << NR_WEIGHT_TBL_G4)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[5], 10) << NR_WEIGHT_TBL_G5)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[6], 10) << NR_WEIGHT_TBL_G6);
    isp->reg_blkaddr2[27] = cmd;

    //raw_reg_29
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_gtbl[7], 10) << NR_WEIGHT_TBL_G7)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[8], 10) << NR_WEIGHT_TBL_G8)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[9], 10) << NR_WEIGHT_TBL_G9);
    isp->reg_blkaddr2[28] = cmd;

    //raw_reg_30
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_gtbl[10], 10) << NR_WEIGHT_TBL_G10)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[11], 10) << NR_WEIGHT_TBL_G11)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[12], 10) << NR_WEIGHT_TBL_G12);
    isp->reg_blkaddr2[29] = cmd;

    //raw_reg_31
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_gtbl[13], 10) << NR_WEIGHT_TBL_G13)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[14], 10) << NR_WEIGHT_TBL_G14)
    	| (LOW_BITS(nr1->nr1_weight_gtbl[15], 10) << NR_WEIGHT_TBL_G15);
    isp->reg_blkaddr2[30] = cmd;

    //raw_reg_32
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_gtbl[16], 10) << NR_WEIGHT_TBL_G16)
    	| (LOW_BITS(nr1->nr1_weight_btbl[0], 10) << NR_WEIGHT_TBL_B0)
    	| (LOW_BITS(nr1->nr1_weight_btbl[1], 10) << NR_WEIGHT_TBL_B1);
    isp->reg_blkaddr2[31] = cmd;

    //raw_reg_33
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_btbl[2], 10) << NR_WEIGHT_TBL_B2)
    	| (LOW_BITS(nr1->nr1_weight_btbl[3], 10) << NR_WEIGHT_TBL_B3)
    	| (LOW_BITS(nr1->nr1_weight_btbl[4], 10) << NR_WEIGHT_TBL_B4);
    isp->reg_blkaddr2[32] = cmd;

    //raw_reg_34
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_btbl[5], 10) << NR_WEIGHT_TBL_B5)
    	| (LOW_BITS(nr1->nr1_weight_btbl[6], 10) << NR_WEIGHT_TBL_B6)
    	| (LOW_BITS(nr1->nr1_weight_btbl[7], 10) << NR_WEIGHT_TBL_B7);
    isp->reg_blkaddr2[33] = cmd;

    //raw_reg_35
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_btbl[8], 10) << NR_WEIGHT_TBL_B8)
    	| (LOW_BITS(nr1->nr1_weight_btbl[9], 10) << NR_WEIGHT_TBL_B9)
    	| (LOW_BITS(nr1->nr1_weight_btbl[10], 10) << NR_WEIGHT_TBL_B10);
    isp->reg_blkaddr2[34] = cmd;
    
    //raw_reg_36
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_btbl[11], 10) << NR_WEIGHT_TBL_B11)
    	| (LOW_BITS(nr1->nr1_weight_btbl[12], 10) << NR_WEIGHT_TBL_B12)
    	| (LOW_BITS(nr1->nr1_weight_btbl[13], 10) << NR_WEIGHT_TBL_B13);
    isp->reg_blkaddr2[35] = cmd;

    //raw_reg_37
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_weight_btbl[14], 10) << NR_WEIGHT_TBL_B14)
    	| (LOW_BITS(nr1->nr1_weight_btbl[15], 10) << NR_WEIGHT_TBL_B15)
    	| (LOW_BITS(nr1->nr1_weight_btbl[16], 10) << NR_WEIGHT_TBL_B16);
    isp->reg_blkaddr2[36] = cmd;

    //raw_reg_38
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_lc_lut[0], 10) << NR_LC_LUT_0)
    	| (LOW_BITS(nr1->nr1_lc_lut[1], 10) << NR_LC_LUT_1)
    	| (LOW_BITS(nr1->nr1_lc_lut[2], 10) << NR_LC_LUT_2);
    isp->reg_blkaddr2[37] = cmd;

    //raw_reg_39
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_lc_lut[3], 10) << NR_LC_LUT_3)
    	| (LOW_BITS(nr1->nr1_lc_lut[4], 10) << NR_LC_LUT_4)
    	| (LOW_BITS(nr1->nr1_lc_lut[5], 10) << NR_LC_LUT_5);
    isp->reg_blkaddr2[38] = cmd;

    //raw_reg_40
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_lc_lut[6], 10) << NR_LC_LUT_6)
	    | (LOW_BITS(nr1->nr1_lc_lut[7], 10) << NR_LC_LUT_7)
	    | (LOW_BITS(nr1->nr1_lc_lut[8], 10) << NR_LC_LUT_8);
    isp->reg_blkaddr2[39] = cmd;
    //raw_reg_41
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_lc_lut[9], 10) << NR_LC_LUT_9)
    	| (LOW_BITS(nr1->nr1_lc_lut[10], 10) << NR_LC_LUT_10)
    	| (LOW_BITS(nr1->nr1_lc_lut[11], 10) << NR_LC_LUT_11);
    isp->reg_blkaddr2[40] = cmd;

    //raw_reg_42
    cmd = 0;
    cmd = (LOW_BITS(nr1->nr1_lc_lut[12], 10) << NR_LC_LUT_12)
    	| (LOW_BITS(nr1->nr1_lc_lut[13], 10) << NR_LC_LUT_13)
    	| (LOW_BITS(nr1->nr1_lc_lut[14], 10) << NR_LC_LUT_14);
    isp->reg_blkaddr2[41] = cmd;

    //raw_reg_43
    cmd = 0;
    cmd = isp->reg_blkaddr2[42];
    CLEAR_BITS(cmd, 0, 24);
    cmd |= (LOW_BITS(nr1->nr1_lc_lut[15], 10) << NR_LC_LUT_15)
    	|  (LOW_BITS(nr1->nr1_lc_lut[16], 10) << NR_LC_LUT_16)
    	|  (LOW_BITS(nr1->nr1_k, 4) << NR_K);
        //(nr1->gb_en_th << GB_EN_TH);
    isp->reg_blkaddr2[42] = cmd;
		
    //enable
    cmd = 0;                                            //reg5
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, NR_EN, 1);
    cmd |= (LOW_BITS(nr1->nr1_enable, 1) << NR_EN);
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

	ak_isp_vo_update_setting();

    return 0;
}

/**
 * @brief: set raw noise remove param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *nr1:  raw noise remove  param
 */
int ak_isp_vp_set_nr1_attr(AK_ISP_NR1_ATTR *p_nr1)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->nr1_para),p_nr1,sizeof(AK_ISP_NR1_ATTR));
	//ak_isp_vo_update_setting();
	isp->linkage_para_update_flag =1;

    return 0;
}


/**
 * @brief: get raw noise remove param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *nr1:  raw noise remove  param
 */
int ak_isp_vp_get_nr1_attr(AK_ISP_NR1_ATTR *p_nr1)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_nr1,&(isp->nr1_para),sizeof(AK_ISP_NR1_ATTR));

    return 0;
}

/**
 * @brief: set green balance param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *gb:  green balance param
 */
int _set_gb_attr( AK_ISP_GB *gb)
{
    unsigned long cmd = 0;

    //raw_reg_43    
    cmd = 0;
    cmd = isp->reg_blkaddr2[42];
    CLEAR_BITS(cmd, GB_EN_TH, 8);
    cmd |= (LOW_BITS(gb->gb_en_th, 8) << GB_EN_TH);
    isp->reg_blkaddr2[42] = cmd;

    //raw_reg_44
     cmd = 0;
    cmd = isp->reg_blkaddr2[43];
    CLEAR_BITS(cmd, 0, 14);
    cmd |= (LOW_BITS(gb->gb_threshold, 10) << GB_TH)
    	|  (LOW_BITS(gb->gb_kstep, 4) << GB_KSTEP);
    isp->reg_blkaddr2[43] = cmd;
        
   	//enable
    cmd = 0;
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, GB_EN, 1);
    cmd |= (LOW_BITS(gb->gb_enable, 1) << GB_EN);
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

	ak_isp_vo_update_setting();

    return 0;
}

/**
 * @brief: set green balance param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_gb:  green balance param
 */
int ak_isp_vp_set_gb_attr( AK_ISP_GB_ATTR *p_gb)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->gb_para),p_gb,sizeof(AK_ISP_GB_ATTR));
	//ak_isp_vo_update_setting();
	isp->linkage_para_update_flag =1;

    return 0;
}

/**
 * @brief: get green balance param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_gb:  green balance param
 */
int ak_isp_vp_get_gb_attr(AK_ISP_GB_ATTR *p_gb)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_gb,&(isp->gb_para),sizeof(AK_ISP_GB_ATTR));

    return 0;
}

/**
 * @brief: set demosaic param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_demo:  demosaic param
 */
int ak_isp_vp_set_demo_attr(AK_ISP_DEMO_ATTR *p_demo)
{
    unsigned long cmd;
    
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->demo_para),p_demo,sizeof(AK_ISP_DEMO_ATTR));

    //raw_reg_44   regblk2
    cmd = 0;
    cmd = LOW_BITS(isp->reg_blkaddr2[43], 14);
    cmd |= (LOW_BITS(p_demo->dm_rg_thre, 10) << DM_RG_TH) 
    	|  (LOW_BITS(p_demo->dm_rg_gain, 8) << DM_RG_GAIN);
    isp->reg_blkaddr2[43]= cmd;

    //raw_reg_45   regblk2
    cmd = 0;
    cmd = (LOW_BITS(p_demo->dm_hf_th1, 10) << DM_HF_TH1)
    	| (LOW_BITS(p_demo->dm_hf_th2, 10) << DM_HF_TH2)
    	| (LOW_BITS(p_demo->dm_bg_thre, 10) << DM_BG_TH);
    isp->reg_blkaddr2[44] = cmd;

    //raw_reg_46  regblk2
    cmd = 0;
    cmd = (LOW_BITS(p_demo->dm_bg_gain, 8) << DM_BG_GAIN)
    	| (LOW_BITS(p_demo->dm_HV_th, 8) << DM_HV_TH)
    	| (LOW_BITS(p_demo->dm_gr_gain, 8) << DM_GR_GAIN)
    	| (LOW_BITS(p_demo->dm_gb_gain, 8) << DM_GB_GAIN);
    isp->reg_blkaddr2[45] = cmd;
    
	ak_isp_vo_update_setting();

    return 0;
}

/**
 * @brief: get demosaic param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_demo:  demosaic param
 */
int ak_isp_vp_get_demo_attr(AK_ISP_DEMO_ATTR *p_demo)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_demo,&(isp->demo_para),sizeof(AK_ISP_DEMO_ATTR));

    return 0;
}

/**
 * @brief: set color correct param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *cc:  color correct param
 */
int _set_ccm_attr(AK_ISP_CCM *cc)   
{
    unsigned long cmd;

    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->ccm_info.ccm_current), cc, sizeof(AK_ISP_CCM));

	//raw_reg_47
	cmd = 0;
    cmd = isp->reg_blkaddr2[46];
	CLEAR_BITS(cmd, 0, 23);
	cmd |= (LOW_BITS(cc->cc_cnoise_yth, 8) << CC_CC_CNOISE_TH)
		|  (LOW_BITS(cc->cc_cnoise_gain, 8) << CC_CC_CNOISE_GAIN)
		|  (LOW_BITS(cc->cc_cnoise_slop, 7) << CC_CC_CNOISE_SLOP);
	isp->reg_blkaddr2[46] = cmd;
	
    //raw_reg_48
    cmd = 0;
    cmd = isp->reg_blkaddr2[47];
    CLEAR_BITS(cmd, 12, 20);
    cmd |= (LOW_BITS(cc->ccm[0][0], 12) << CC_CCM_RR)
    	|  (LOW_BITS(cc->ccm[0][1], 8) << CC_CCM_RG_L8);
    isp->reg_blkaddr2[47] = cmd;

    //raw_reg_49
    cmd = 0;
    cmd = (LOW_BITS((cc->ccm[0][1] >> 8), 4) << CC_CCM_RG_H4)
    	| (LOW_BITS(cc->ccm[0][2], 12) << CC_CCM_RB)
    	| (LOW_BITS(cc->ccm[1][0], 12) << CC_CCM_GR)
    	| (LOW_BITS(cc->ccm[1][1], 4) << CC_CCM_GG_L4);
    isp->reg_blkaddr2[48] = cmd;

    //raw_reg_50
    cmd = 0;
    cmd = (LOW_BITS((cc->ccm[1][1] >> 4), 8) << CC_CCM_GG_H8)
    	| (LOW_BITS(cc->ccm[1][2], 12) << CC_CCM_GB)
    	| (LOW_BITS(cc->ccm[2][0], 12) << CC_CCM_BR);
    isp->reg_blkaddr2[49] = cmd;

    //raw_reg_51
    cmd = 0;
    cmd = (LOW_BITS(cc->ccm[2][1], 12) << CC_CCM_BG)
    	| (LOW_BITS(cc->ccm[2][2], 12) << CC_CCM_BB);
    isp->reg_blkaddr2[50] = cmd;

	//enable   
    cmd = 0;                                            //reg5
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, CC_EN, 1);
    cmd |= (LOW_BITS(cc->cc_enable, 1) << CC_EN);
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

	ak_isp_vo_update_setting();

    return 0;   
}

/**
 * @brief: set color correct param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_ccm:  color correct param
 */
int ak_isp_vp_set_ccm_attr(AK_ISP_CCM_ATTR *p_ccm)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->ccm_para),p_ccm,sizeof(AK_ISP_CCM_ATTR));
	//ak_isp_vo_update_setting();
	isp->linkage_ccm_update_flag =1;

    return 0;
}

/**
 * @brief: set color correct param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_ccm:  color correct param
 */
int ak_isp_vp_get_ccm_attr(AK_ISP_CCM_ATTR *p_ccm)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_ccm,&(isp->ccm_para),sizeof(AK_ISP_CCM_ATTR));

    return 0;
}

/**
 * @brief: set wdr  param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_wdr: wdr param
 */
int _set_wdr_attr(AK_ISP_WDR*p_wdr)
{
    int i = 0,j=0,k=1;
    unsigned long cmd = 0;
    int hdr_cnoise_suppress_slop;
    isp_dbg("%s enter.\n", __func__);

	if(p_wdr->wdr_enable)
	{
		if(p_wdr->hdr_cnoise_suppress_yth2 == p_wdr->hdr_cnoise_suppress_yth1)
			hdr_cnoise_suppress_slop = 128;
		else
			hdr_cnoise_suppress_slop = 1024/(p_wdr->hdr_cnoise_suppress_yth2-p_wdr->hdr_cnoise_suppress_yth1);
		
		for (i=1; i<129; i+=2)
		{			
		    cmd = 0;
		    cmd = (p_wdr->area_tb1[k]) |
		        (p_wdr->area_tb2[k] << 10) |
		        (p_wdr->area_tb3[k] << 20);
			
		    k++;
		    j = i-1;
		    isp->cfg_mem_blkaddr4[j] = cmd;
		}

		k = 1;
		for (i=2; i<129; i+=2)
		{
			cmd = 0;
			cmd = (p_wdr->area_tb4[k]) |
				  (p_wdr->area_tb5[k] << 10) |
				  (p_wdr->area_tb6[k] << 20);

			k++;
			j = i-1;
			isp->cfg_mem_blkaddr4[j] = cmd;
		}
		
	    //yuv_reg_3
		cmd = 0;
		cmd |= (LOW_BITS(p_wdr->wdr_th1, 8) << HDR_TH1) 
			|  (LOW_BITS(p_wdr->wdr_th2, 8) << HDR_TH2) 
			|  (LOW_BITS(p_wdr->wdr_th3, 8) << HDR_TH3) 
			|  (LOW_BITS(p_wdr->wdr_th4, 8) << HDR_TH4);
		isp->reg_blkaddr3[2] = cmd;
		 
		//yuv_reg_4
		cmd = 0;
		cmd = isp->reg_blkaddr3[3];
		CLEAR_BITS(cmd, 0, 8);
		cmd |= (LOW_BITS(p_wdr->wdr_th5, 8) << HDR_TH5);
		isp->reg_blkaddr3[3] = cmd;
		 
		//yuv_reg_5
		cmd = 0;
		cmd = isp->reg_blkaddr3[4];
		CLEAR_BITS(cmd, 12, 18);
		cmd |= (LOW_BITS(hdr_cnoise_suppress_slop, 8) << HDR_CNOISE_SUPPRESS_SLOP)
			|  (LOW_BITS(p_wdr->hdr_cnoise_suppress_yth1, 10) << HDR_CNOISE_SUPPRESS_YTH1);
		isp->reg_blkaddr3[4]= cmd;
		 
		//yuv_reg_6
		cmd = 0;
		cmd = isp->reg_blkaddr3[5];
		CLEAR_BITS(cmd, 0, 25);
		cmd |= (LOW_BITS(p_wdr->hdr_cnoise_suppress_yth2, 10) << HDR_CNOISE_SUPPRESS_YTH2) 
			|  (LOW_BITS(p_wdr->hdr_cnoise_suppress_gain, 10) << HDR_CNOISE_SUPPRESS_GAIN) 
			|  (LOW_BITS(p_wdr->hdr_uv_adjust_level, 5) << HDR_UV_ADJUST_LEVEL);
		isp->reg_blkaddr3[5] = cmd;
	}
	
	//enable
	cmd = 0; 
	cmd = isp->reg_blkaddr5[R5_ENABLE];
	CLEAR_BITS(cmd, HDR_UV_ADJUST_EN, 2);
	cmd |= ((LOW_BITS(p_wdr->hdr_uv_adjust_enable, 1) << HDR_UV_ADJUST_EN)
		|  (LOW_BITS(p_wdr->wdr_enable, 1) << HDR_EN));  
	isp->reg_blkaddr5[R5_ENABLE] = cmd;

	ak_isp_vo_update_setting();

    return 0; 
}

/**
 * @brief: set wdr  param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_wdr: wdr param
 */
int ak_isp_vp_set_wdr_attr(AK_ISP_WDR_ATTR *p_wdr)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->wdr_para), p_wdr,sizeof(AK_ISP_WDR_ATTR));
    isp->linkage_para_update_flag = 1;

    return 0;
}

/**
 * @brief: get wdr  param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_wdr: wdr param
 */
int ak_isp_vp_get_wdr_attr(AK_ISP_WDR_ATTR *p_wdr)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_wdr,&(isp->wdr_para), sizeof(AK_ISP_WDR_ATTR));

    return 0;
}

/**
 * @brief: set wdr other param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_wdr_ex: wdr other param
 */
int ak_isp_vp_set_wdr_ex_attr(AK_ISP_WDR_EX_ATTR *p_wdr_ex)
{
    isp_dbg("%s enter.\n", __func__);
    //code was moved to ak_isp_vi_set_crop, auto caculate with cut_width/cut_height

    return 0;
}

/**
 * @brief: get wdr other param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_wdr_ex: wdr other param
 */
int ak_isp_vp_get_wdr_ex_attr(AK_ISP_WDR_EX_ATTR *p_wdr_ex)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_wdr_ex,&(isp->wdr_ex_para), sizeof(AK_ISP_WDR_EX_ATTR));

    return 0;
}

/**
 * @brief: set yuv noise remove param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *nr2: noise remove param
 */
int _set_nr2_attr(AK_ISP_NR2 *nr2)
{
	unsigned long cmd;
    isp_dbg("%s enter.\n", __func__);

    //yuv_reg_23
    cmd = 0;
    cmd = (LOW_BITS(nr2->nr2_weight_tbl[1], 10) << YNR_WEIGHT_TBL_1) 
    	| (LOW_BITS(nr2->nr2_weight_tbl[2], 10) << YNR_WEIGHT_TBL_2) 
    	| (LOW_BITS(nr2->nr2_weight_tbl[3], 10) << YNR_WEIGHT_TBL_3);
    isp->reg_blkaddr3[22] =cmd;
    
    //yuv_reg_24
    cmd = 0;
    cmd = (LOW_BITS(nr2->nr2_weight_tbl[4], 10) << YNR_WEIGHT_TBL_4) 
    	| (LOW_BITS(nr2->nr2_weight_tbl[5], 10) << YNR_WEIGHT_TBL_5) 
    	| (LOW_BITS(nr2->nr2_weight_tbl[6], 10) << YNR_WEIGHT_TBL_6);
     isp->reg_blkaddr3[23] =cmd;

    //yuv_reg_25
    cmd = 0;
    cmd = (LOW_BITS(nr2->nr2_weight_tbl[7], 10) << YNR_WEIGHT_TBL_7) 
    	| (LOW_BITS(nr2->nr2_weight_tbl[8], 10) << YNR_WEIGHT_TBL_8) 
    	| (LOW_BITS(nr2->nr2_weight_tbl[9], 10) << YNR_WEIGHT_TBL_9);
	isp->reg_blkaddr3[24] =cmd;

    //yuv_reg_26
    cmd = 0;
    cmd = (LOW_BITS(nr2->nr2_weight_tbl[10], 10) << YNR_WEIGHT_TBL_10) 
    	| (LOW_BITS(nr2->nr2_weight_tbl[11], 10) << YNR_WEIGHT_TBL_11) 
    	| (LOW_BITS(nr2->nr2_weight_tbl[12], 10) << YNR_WEIGHT_TBL_12);
    isp->reg_blkaddr3[25] =cmd;

    //yuv_reg_27
    cmd = 0;
    cmd = (LOW_BITS(nr2->nr2_weight_tbl[13], 10) << YNR_WEIGHT_TBL_13) 
    	| (LOW_BITS(nr2->nr2_weight_tbl[14], 10) << YNR_WEIGHT_TBL_14) 
    	| (LOW_BITS(nr2->nr2_weight_tbl[15], 10) << YNR_WEIGHT_TBL_15);
    isp->reg_blkaddr3[26] =cmd;

    //yuv_reg_28
    cmd = 0;
	cmd = isp->reg_blkaddr3[27];
	CLEAR_BITS(cmd, 0, 24);
	CLEAR_BITS(cmd, 30, 2);
    cmd |= (LOW_BITS(nr2->nr2_weight_tbl[16], 10) << YNR_WEIGHT_TBL_16) 
    	|  (LOW_BITS(nr2->y_dpc_th, 10) << YNR_Y_DPC_TH)
    	|  (LOW_BITS(nr2->nr2_k, 4) << YNR_K)
    	|  (LOW_BITS(nr2->y_black_dpc_enable, 1) << YNR_Y_BLACK_DPC_EN)
    	|  (LOW_BITS(nr2->y_white_dpc_enable, 1) << YNR_Y_WHITE_DPC_EN);
    isp->reg_blkaddr3[27] =cmd;

	
    //enable
    cmd = 0;	//reg5
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, Y_DPC_EN, 2);
    cmd |= (LOW_BITS(nr2->y_dpc_enable, 1) << Y_DPC_EN)
    	|  (LOW_BITS(nr2->nr2_enable, 1) << YNR_EN);
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

	ak_isp_vo_update_setting();

    return 0;
}

/**
 * @brief: set yuv noise remove param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_nr2: noise remove param
 */
int ak_isp_vp_set_nr2_attr(AK_ISP_NR2_ATTR *p_nr2)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->nr2_para),p_nr2,sizeof(AK_ISP_NR2_ATTR));
	//ak_isp_vo_update_setting();
	isp->linkage_para_update_flag =1;

    return 0;
}

/**
 * @brief: get yuv noise remove param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_nr2: noise remove param
 */
int ak_isp_vp_get_nr2_attr(AK_ISP_NR2_ATTR *p_nr2)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_nr2,&(isp->nr2_para),sizeof(AK_ISP_NR2_ATTR));

    return 0;
}

/**
 * @brief: set 3d noise remove param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *_3d_nr: 3d noise remove param
 */
int _set_3d_nr_attr(AK_ISP_3D_NR  *_3d_nr)
{
	unsigned long cmd;
	int t_y_k1, t_y_k2,t_y_ac_th,t_uv_ac_th;
	int t_y_diffth_slop = 0;
	int t_uv_diffth_slop = 0;
	AK_ISP_3D_NR *_3d_nr_still;
	AK_ISP_3D_NR_STAT_INFO *tnr_stat;
#ifndef CONFIG_CPU_AK3918EV200
	/* Feeds TNR_T_Y_EX_K, which this part does not have - see the register
	 * write below. The field it reads is gone from AK_ISP_3D_NR here. */
	int tnr_t_y_ex_k_cfg;
#endif
	
	isp_dbg("%s enter.\n", __func__);

	_3d_nr_still = &isp->_3d_nr_para.linkage_3d_nr[8];
	tnr_stat = &isp->_3d_nr_stat_para;

	//yuv_reg_9
	t_y_k2 = _3d_nr->t_y_k2;
	if(_3d_nr->t_y_k2<_3d_nr_still->t_y_k2)
	{
		t_y_k2 = (_3d_nr->t_y_k2 *tnr_stat->MD_level+_3d_nr_still->t_y_k2*(16-tnr_stat->MD_level ))/16;
#ifndef CONFIG_CPU_AK3918EV200
		tnr_t_y_ex_k_cfg = _3d_nr_still->t_y_k2-t_y_k2;
#endif
	}
#ifndef CONFIG_CPU_AK3918EV200
	else
		tnr_t_y_ex_k_cfg = _3d_nr->tnr_t_y_ex_k_cfg;
#endif
	cmd = 0;
	cmd = isp->reg_blkaddr3[8];
#ifdef CONFIG_CPU_AK3918EV200
	/* No TNR_T_Y_EX_K at bits 19-25 on this part - stock clears 16..18 only,
	 * and so does the EV33x build. Writing it puts 7 bits into an undefined
	 * region of YUV_REG_9 on every 3D-NR update. */
	CLEAR_BITS(cmd, 16, 3);
	cmd |= (LOW_BITS(_3d_nr->uv_min_enable, 1) << TNR_UV_MIN_EN)
		|  (LOW_BITS(_3d_nr->tnr_refFrame_format, 2) << TNR_COMPRESS_FORMAT);
#else
	CLEAR_BITS(cmd, 16, 10);
	cmd |= (LOW_BITS(_3d_nr->uv_min_enable, 1) << TNR_UV_MIN_EN)
		|  (LOW_BITS(_3d_nr->tnr_refFrame_format, 2) << TNR_COMPRESS_FORMAT)
		|  (LOW_BITS(tnr_t_y_ex_k_cfg,7)<<TNR_T_Y_EX_K);//lz0499 9_6
#endif
	isp->reg_blkaddr3[8] = cmd;
	
	//yuv_reg_10
	cmd = 0;
	cmd = (LOW_BITS(_3d_nr->ynr_weight_tbl[1], 10) << TNR_YNR_WEIGHT_TBL_1)
		| (LOW_BITS(_3d_nr->ynr_weight_tbl[2], 10) << TNR_YNR_WEIGHT_TBL_2)
		| (LOW_BITS(_3d_nr->ynr_weight_tbl[3], 10) << TNR_YNR_WEIGHT_TBL_3);
	isp->reg_blkaddr3[9] = cmd;

	//yuv_reg_11
	cmd = 0;
	cmd = (LOW_BITS(_3d_nr->ynr_weight_tbl[4], 10) << TNR_YNR_WEIGHT_TBL_4)
		| (LOW_BITS(_3d_nr->ynr_weight_tbl[5], 10) << TNR_YNR_WEIGHT_TBL_5)
		| (LOW_BITS(_3d_nr->ynr_weight_tbl[6], 10) << TNR_YNR_WEIGHT_TBL_6);
	isp->reg_blkaddr3[10] = cmd;

	//yuv_reg_12
	cmd = 0;
	cmd = (LOW_BITS(_3d_nr->ynr_weight_tbl[7], 10) << TNR_YNR_WEIGHT_TBL_7)
		| (LOW_BITS(_3d_nr->ynr_weight_tbl[8], 10) << TNR_YNR_WEIGHT_TBL_8)
		| (LOW_BITS(_3d_nr->ynr_weight_tbl[9], 10) << TNR_YNR_WEIGHT_TBL_9);
	isp->reg_blkaddr3[11] = cmd;

	//yuv_reg_13
	cmd = 0;
	cmd = (LOW_BITS(_3d_nr->ynr_weight_tbl[10], 10) << TNR_YNR_WEIGHT_TBL_10)
		| (LOW_BITS(_3d_nr->ynr_weight_tbl[11], 10) << TNR_YNR_WEIGHT_TBL_11)
		| (LOW_BITS(_3d_nr->ynr_weight_tbl[12], 10) << TNR_YNR_WEIGHT_TBL_12);
	isp->reg_blkaddr3[12] = cmd;

	//yuv_reg_14
	cmd = 0;
	cmd = (LOW_BITS(_3d_nr->ynr_weight_tbl[13], 10) << TNR_YNR_WEIGHT_TBL_13)
		| (LOW_BITS(_3d_nr->ynr_weight_tbl[14], 10) << TNR_YNR_WEIGHT_TBL_14)
		| (LOW_BITS(_3d_nr->ynr_weight_tbl[15], 10) << TNR_YNR_WEIGHT_TBL_15);
	isp->reg_blkaddr3[13] = cmd;

	//yuv_reg_15
	cmd = 0;
	cmd = (LOW_BITS(_3d_nr->ynr_weight_tbl[16], 10) << TNR_YNR_WEIGHT_TBL_16)
		| (LOW_BITS(_3d_nr->ynr_diff_shift, 1) << TNR_YNR_DIFF_SHIFT)
		| (LOW_BITS(_3d_nr->ynr_k, 4)  << TNR_YNR_K)
		| (LOW_BITS(_3d_nr->ylp_k, 4) << TNR_YLP_K)
		| (LOW_BITS(_3d_nr->uvnr_k, 4) << TNR_UVNR_K)
		| (LOW_BITS(_3d_nr->uvlp_k, 4) << TNR_UVLP_K);	   
	isp->reg_blkaddr3[14] = cmd;


	if(_3d_nr->t_y_mf_th2 == _3d_nr->t_y_mf_th1)
		_3d_nr->t_y_mf_th1 = _3d_nr->t_y_mf_th2 - 1;
	//yuv_reg_16
	t_y_diffth_slop =  (((_3d_nr->t_y_diffth_k1 - _3d_nr->t_y_diffth_k2 )*512)
		/ (_3d_nr->t_y_mf_th2 - _3d_nr->t_y_mf_th1) +1)/2;
	cmd = 0;
	cmd = (LOW_BITS(_3d_nr->t_y_th1, 8) << TNR_T_Y_TH1)
		| (LOW_BITS(_3d_nr->t_y_diffth_k1, 8) << TNR_T_Y_DIFFTH_K1)
		| (LOW_BITS(_3d_nr->t_y_diffth_k2, 8) << TNR_T_Y_DIFFTH_K2)
		| (LOW_BITS(t_y_diffth_slop, 8) << TNR_T_Y_DIFFTH_SLOP);
	isp->reg_blkaddr3[15] = cmd;

	//yuv_reg_17
	t_y_k1 = _3d_nr->t_y_k1;
	if(_3d_nr->t_y_k1<_3d_nr_still->t_y_k1)
		t_y_k1 = (_3d_nr->t_y_k1 *tnr_stat->MD_level+_3d_nr_still->t_y_k1*(16-tnr_stat->MD_level ))/16;
	t_y_k2 = _3d_nr->t_y_k2;
	if(_3d_nr->t_y_k2<_3d_nr_still->t_y_k2)
		t_y_k2 = (_3d_nr->t_y_k2 *tnr_stat->MD_level+_3d_nr_still->t_y_k2*(16-tnr_stat->MD_level ))/16;
	t_y_ac_th = (_3d_nr->t_y_ac_th *tnr_stat->MD_level+_3d_nr_still->t_y_ac_th*(16-tnr_stat->MD_level ))/16;
	cmd = 0;
	cmd = (LOW_BITS(t_y_k1, 7) << TNR_T_Y_K1)
		| (LOW_BITS(t_y_k2, 7) << TNR_T_Y_K2)
		| (LOW_BITS(_3d_nr->t_y_kslop, 7) << TNR_T_Y_KSLOP)
		| (LOW_BITS(t_y_ac_th, 10) << TNR_T_Y_AC_TH);
	isp->reg_blkaddr3[16] = cmd;

	//yuv_reg_18
	cmd = 0;
	//cmd = (LOW_BITS(_3d_nr->t_y_minstep, 5) << TNR_T_Y_MINSTEP)
	cmd = (LOW_BITS(2, 5) << TNR_T_Y_MINSTEP)
		| (LOW_BITS(_3d_nr->t_y_mf_th1, 13) << TNR_T_Y_MF_TH1)
		| (LOW_BITS(_3d_nr->t_y_mf_th2, 13) << TNR_T_Y_MF_TH2);
	isp->reg_blkaddr3[17] = cmd;

	//yuv_reg_19
	t_uv_ac_th = (_3d_nr->t_uv_ac_th *tnr_stat->MD_level+_3d_nr_still->t_uv_ac_th*(16-tnr_stat->MD_level ))/16;
	cmd = 0;
	cmd = (LOW_BITS(_3d_nr->t_y_mc_k, 5) << TNR_T_Y_MC_K)
		| (LOW_BITS(_3d_nr->t_uv_k, 7) << TNR_T_UV_K)
		| (LOW_BITS(_3d_nr->t_uv_minstep, 5) << TNR_T_UV_MINSTEP)
		| (LOW_BITS(_3d_nr->t_uv_mc_k, 5) << TNR_T_UV_MC_K)
		| (LOW_BITS(t_uv_ac_th, 10) << TNR_T_UV_AC_TH);
	isp->reg_blkaddr3[18] = cmd;
	
	//yuv_reg_20
	cmd = 0;
	cmd = (LOW_BITS(_3d_nr->t_uv_mf_th1, 13) << TNR_T_UV_MF_TH1)
		| (LOW_BITS(_3d_nr->t_uv_mf_th2, 13) << TNR_T_UV_MF_TH2);
	isp->reg_blkaddr3[19] = cmd;


	if(_3d_nr->t_uv_mf_th2 == _3d_nr->t_uv_mf_th1)
			_3d_nr->t_uv_mf_th1 = _3d_nr->t_uv_mf_th2 - 1;

	//yuv_reg_21
	t_uv_diffth_slop =  (((_3d_nr->t_uv_diffth_k1 - _3d_nr->t_uv_diffth_k2 ) * 512)
		/ (_3d_nr->t_uv_mf_th2 - _3d_nr->t_uv_mf_th1)+1)/2;
	cmd = 0;
	cmd = isp->reg_blkaddr3[20];
	CLEAR_BITS(cmd, 0, 24);
	cmd |= (LOW_BITS(_3d_nr->t_uv_diffth_k1,8) << TNR_T_UV_DIFFTH_K1)
		| (LOW_BITS(_3d_nr->t_uv_diffth_k2,8) << TNR_T_UV_DIFFTH_K2)
		| (LOW_BITS(t_uv_diffth_slop,8) << TNR_T_UV_DIFFTH_SLOP);
	isp->reg_blkaddr3[20] = cmd;

	//endble
	cmd = 0;
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, TNR_Y_EN, 6);
    cmd |=  (LOW_BITS(_3d_nr->tnr_y_enable, 1) << TNR_Y_EN)
    	|   (LOW_BITS(_3d_nr->updata_ref_y, 1) << TNR_UPDATA_REF_Y)
    	|   (LOW_BITS(_3d_nr->tnr_uv_enable, 1) << TNR_UV_EN)
    	|   (LOW_BITS(_3d_nr->updata_ref_uv, 1) << TNR_UPDATA_REF_UV)
    	|   (LOW_BITS(_3d_nr->y_2dnr_enable, 1) << TNR_Y_2DNR_EN)
    	|   (LOW_BITS(_3d_nr->uv_2dnr_enable, 1) <<TNR_UV_2DNR_EN);


    if (isp->_3dnr_reset) {
		CLEAR_BITS(cmd, TNR_UV_EN, 1);
		CLEAR_BITS(cmd, TNR_Y_EN, 1);
	}
    
	isp->reg_blkaddr5[R5_ENABLE] = cmd;
	
	ak_isp_vo_update_setting();
	
    return 0;	
}

/**
 * @brief: set 3d noise remove param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_3d_nr: 3d noise remove param
 */
int ak_isp_vp_set_3d_nr_attr(AK_ISP_3D_NR_ATTR *p_3d_nr)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->_3d_nr_para),p_3d_nr,sizeof(AK_ISP_3D_NR_ATTR));
	//ak_isp_vo_update_setting();
	isp->linkage_para_update_flag =1;

    return 0;
}

/**
 * @brief: get 3d noise remove param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_3d_nr: 3d noise remove param
 */
int ak_isp_vp_get_3d_nr_attr(AK_ISP_3D_NR_ATTR *p_3d_nr)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_3d_nr,&(isp->_3d_nr_para),sizeof(AK_ISP_3D_NR_ATTR));

    return 0;
}

/**
 * @brief: get 3d noise remove statics param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_3d_nr_stat_info: 3d noise remove statics param
 */
int ak_isp_vp_get_3d_nr_stat_info(AK_ISP_3D_NR_STAT_INFO * p_3d_nr_stat_info)
{
	isp_dbg("%s enter.\n", __func__);
	memcpy(p_3d_nr_stat_info,&(isp->_3d_nr_stat_para), sizeof(AK_ISP_3D_NR_STAT_INFO));

	return 0;
}

/**
 * @brief: set 3d noise remove reference param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_ref: 3d noise remove referenc param
 */
int ak_isp_vp_set_3d_nr_ref_addr(AK_ISP_3D_NR_REF_ATTR *p_ref)
{
    unsigned long cmd = 0;
    
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->_3d_nr_ref_para),p_ref,sizeof(AK_ISP_3D_NR_REF_ATTR));

    //addr_reg_31   
    cmd = 0;
    cmd = (LOW_BITS(p_ref->yaddr_3d, 30) << TNR_YADDR);
    isp->reg_blkaddr1[R1_TNR_YADDR] = cmd;

    //addr_reg_32
    cmd = 0;
    cmd = (LOW_BITS(p_ref->ysize_3d, 30) << TNR_YSIZE);
    isp->reg_blkaddr1[R1_TNR_YADDR + 1] = cmd;

    //addr_reg_33
    cmd = 0;
    cmd = (LOW_BITS(p_ref->uaddr_3d, 30) << TNR_UADDR);
    isp->reg_blkaddr1[R1_TNR_YADDR + 2] = cmd;

    //addr_reg_34
    cmd = 0;
    cmd = (LOW_BITS(p_ref->usize_3d, 30) << TNR_USIZE);
    isp->reg_blkaddr1[R1_TNR_YADDR + 3] = cmd;

    //addr_reg_35
    cmd = 0;
    cmd = (LOW_BITS(p_ref->vaddr_3d, 30) << TNR_VADDR);
    isp->reg_blkaddr1[R1_TNR_YADDR + 4] = cmd;

    //addr_reg_36
    cmd = 0;
	cmd = isp->reg_blkaddr1[R1_TNR_YADDR + 5];
	CLEAR_BITS(cmd, 0, 30);
    cmd |= (LOW_BITS(p_ref->vsize_3d,30) << TNR_VSIZE);
    isp->reg_blkaddr1[R1_TNR_YADDR + 5] = cmd;

	ak_isp_vo_update_setting();

    return 0;
}

/**
 * @brief: get 3d noise remove reference param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_ref: 3d noise remove referenc param
 */
int ak_isp_vp_get_3d_nr_ref_addr(AK_ISP_3D_NR_REF_ATTR *p_ref)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_ref,&(isp->_3d_nr_ref_para),sizeof(AK_ISP_3D_NR_REF_ATTR));

    return 0;
}

/**
 * @brief: set sharp param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *sharp: sharp param
 */
int _set_sharp_attr(AK_ISP_SHARP *sharp)
{
	unsigned long cmd;
	int i ,j;
	isp_dbg("%s enter.\n", __func__);

    //yuv_reg_30
    cmd = 0 ; 
    cmd  = isp->reg_blkaddr3[29];
    CLEAR_BITS(cmd, 6, 10);
    cmd |= (LOW_BITS(sharp->mf_hpf_k, 7) << YUV_SHARPEN_MF_HPF_K) 
    	|  (LOW_BITS(sharp->sharp_method, 2) << YUV_SHARPEN_METHOD) ;
    isp->reg_blkaddr3[29] = cmd;

    //yuv_reg_32
    cmd = 0;
    cmd = (LOW_BITS(sharp->mf_hpf_shift, 4) << YUV_SHARPEN_MF_HPF_SHIFT) 
    	| (LOW_BITS(sharp->hf_hpf_k, 7) << YUV_SHARPEN_HF_HPF_K) 
    	| (LOW_BITS(sharp->hf_hpf_shift, 4) << YUV_SHARPEN_HF_HPF_SHIFT) 
    	| (LOW_BITS(sharp->sharp_skin_gain_th, 8) << YUV_SHARPEN_SKIN_GAIN_TH) 
    	| (LOW_BITS(sharp->sharp_skin_gain_weaken, 2) << YUV_SHARPEN_SKIN_GAIN_WEEKEN);
    isp->reg_blkaddr3[31] = cmd;
    
    //MF
    j = 0;
    for (i=0; i<256; i+=32)
    {
        cmd = 0;
        cmd = (sharp->MF_HPF_LUT[i] & 0x1FF) |
            ((sharp->MF_HPF_LUT[i+1] & 0x1FF) << 9) |
            ((sharp->MF_HPF_LUT[i+2] & 0x1FF) << 18) |
            ((sharp->MF_HPF_LUT[i+3] & 0x1F) << 27);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;

        cmd = 0;
        cmd = ((sharp->MF_HPF_LUT[i+3] & 0x1E0) >> 5) |
            ((sharp->MF_HPF_LUT[i+4] & 0x1FF) << 4) |
            ((sharp->MF_HPF_LUT[i+5] & 0x1FF) << 13) |
            ((sharp->MF_HPF_LUT[i+6] & 0x1FF) << 22) |
            ((sharp->MF_HPF_LUT[i+7] & 0x1) << 31);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
        
        cmd = 0;
        cmd = ((sharp->MF_HPF_LUT[i+7] & 0x1FE) >> 1) |
            ((sharp->MF_HPF_LUT[i+8] & 0x1FF) << 8) |
            ((sharp->MF_HPF_LUT[i+9] & 0x1FF) << 17) |
            ((sharp->MF_HPF_LUT[i+10] & 0x3F) << 26);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
         j++;
         
        cmd = 0;
        cmd = ((sharp->MF_HPF_LUT[i+10] & 0x1C0) >> 6) |
            ((sharp->MF_HPF_LUT[i+11] & 0x1FF) << 3) |
            ((sharp->MF_HPF_LUT[i+12] & 0x1FF) << 12) |
            ((sharp->MF_HPF_LUT[i+13] & 0x1FF) << 21) |
            ((sharp->MF_HPF_LUT[i+14] & 0x3) << 30);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;

         
        cmd = 0;
        cmd = ((sharp->MF_HPF_LUT[i+14] & 0x1FC) >> 2) |
            ((sharp->MF_HPF_LUT[i+15] & 0x1FF) << 7) |
            ((sharp->MF_HPF_LUT[i+16] & 0x1FF) << 16) |
            ((sharp->MF_HPF_LUT[i+17] & 0x7F) << 25);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;

        cmd = 0;
        cmd = ((sharp->MF_HPF_LUT[i+17] & 0x180) >> 7) |
            ((sharp->MF_HPF_LUT[i+18] & 0x1FF) << 2) |
            ((sharp->MF_HPF_LUT[i+19] & 0x1FF) << 11) |
            ((sharp->MF_HPF_LUT[i+20] & 0x1FF) << 20) |
            ((sharp->MF_HPF_LUT[i+21] & 0x7) << 29);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;

        cmd = 0;
        cmd = ((sharp->MF_HPF_LUT[i+21] & 0x1F8) >> 3) |
            ((sharp->MF_HPF_LUT[i+22] & 0x1FF) << 6) |
            ((sharp->MF_HPF_LUT[i+23] & 0x1FF) << 15) |
            ((sharp->MF_HPF_LUT[i+24] & 0xFF) << 24);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;

        cmd = 0;
        cmd = ((sharp->MF_HPF_LUT[i+24] & 0x100) >> 8) |
            ((sharp->MF_HPF_LUT[i+25] & 0x1FF) << 1) |
            ((sharp->MF_HPF_LUT[i+26] & 0x1FF) << 10) |
            ((sharp->MF_HPF_LUT[i+27] & 0x1FF) << 19) |
            ((sharp->MF_HPF_LUT[i+28] & 0xF) << 28);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;

        cmd = 0;
        cmd = ((sharp->MF_HPF_LUT[i+28] & 0x1F0) >> 4) |
            ((sharp->MF_HPF_LUT[i+29] & 0x1FF) << 5) |
            ((sharp->MF_HPF_LUT[i+30] & 0x1FF) << 14) |
            ((sharp->MF_HPF_LUT[i+31] & 0x1FF) << 23);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
        
    }

    //HF
    for (i=0; i<256; i+=32)
    {
        cmd = 0;
        cmd = (sharp->HF_HPF_LUT[i] & 0x1FF) |
            ((sharp->HF_HPF_LUT[i+1] & 0x1FF) << 9) |
            ((sharp->HF_HPF_LUT[i+2] & 0x1FF) << 18) |
            ((sharp->HF_HPF_LUT[i+3] & 0x1F) << 27);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
        
        cmd = 0;
        cmd = ((sharp->HF_HPF_LUT[i+3] & 0x1E0) >> 5) |
            ((sharp->HF_HPF_LUT[i+4] & 0x1FF) << 4) |
            ((sharp->HF_HPF_LUT[i+5] & 0x1FF) << 13) |
            ((sharp->HF_HPF_LUT[i+6] & 0x1FF) << 22) |
            ((sharp->HF_HPF_LUT[i+7] & 0x1) << 31);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
        
        cmd = 0;
        cmd = ((sharp->HF_HPF_LUT[i+7] & 0x1FE) >> 1) |
            ((sharp->HF_HPF_LUT[i+8] & 0x1FF) << 8) |
            ((sharp->HF_HPF_LUT[i+9] & 0x1FF) << 17) |
            ((sharp->HF_HPF_LUT[i+10] & 0x3F) << 26);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
        
        cmd = 0;
        cmd = ((sharp->HF_HPF_LUT[i+10] & 0x1C0) >> 6) |
            ((sharp->HF_HPF_LUT[i+11] & 0x1FF) << 3) |
            ((sharp->HF_HPF_LUT[i+12] & 0x1FF) << 12) |
            ((sharp->HF_HPF_LUT[i+13] & 0x1FF) << 21) |
            ((sharp->HF_HPF_LUT[i+14] & 0x3) << 30);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
        
        cmd = 0;
        cmd = ((sharp->HF_HPF_LUT[i+14] & 0x1FC) >> 2) |
            ((sharp->HF_HPF_LUT[i+15] & 0x1FF) << 7) |
            ((sharp->HF_HPF_LUT[i+16] & 0x1FF) << 16) |
            ((sharp->HF_HPF_LUT[i+17] & 0x7F) << 25);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
        
        cmd = 0;
        cmd = ((sharp->HF_HPF_LUT[i+17] & 0x180) >> 7) |
            ((sharp->HF_HPF_LUT[i+18] & 0x1FF) << 2) |
            ((sharp->HF_HPF_LUT[i+19] & 0x1FF) << 11) |
            ((sharp->HF_HPF_LUT[i+20] & 0x1FF) << 20) |
            ((sharp->HF_HPF_LUT[i+21] & 0x7) << 29);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
        
        cmd = 0;
        cmd = ((sharp->HF_HPF_LUT[i+21] & 0x1F8) >> 3) |
            ((sharp->HF_HPF_LUT[i+22] & 0x1FF) << 6) |
            ((sharp->HF_HPF_LUT[i+23] & 0x1FF) << 15) |
            ((sharp->HF_HPF_LUT[i+24] & 0xFF) << 24);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
        
        cmd = 0;
        cmd = ((sharp->HF_HPF_LUT[i+24] & 0x100) >> 8) |
            ((sharp->HF_HPF_LUT[i+25] & 0x1FF) << 1) |
            ((sharp->HF_HPF_LUT[i+26] & 0x1FF) << 10) |
            ((sharp->HF_HPF_LUT[i+27] & 0x1FF) << 19) |
            ((sharp->HF_HPF_LUT[i+28] & 0xF) << 28);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
        
        cmd = 0;
        cmd = ((sharp->HF_HPF_LUT[i+28] & 0x1F0) >> 4) |
            ((sharp->HF_HPF_LUT[i+29] & 0x1FF) << 5) |
            ((sharp->HF_HPF_LUT[i+30] & 0x1FF) << 14) |
            ((sharp->HF_HPF_LUT[i+31] & 0x1FF) << 23);
        //*pReg++ = cmd;
        isp->cfg_mem_blkaddr5[j] = cmd;
        j++;
    }
    
    //enable
    cmd = 0;                                            //reg5
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, YUV_SHARPEN_SKIN_DETECT, 2);
    cmd |= ((LOW_BITS(sharp->sharp_skin_detect_enable, 1) << YUV_SHARPEN_SKIN_DETECT)
    	|  (LOW_BITS(sharp->ysharp_enable, 1) << YUV_SHARPEN_EN));
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

	ak_isp_vo_update_setting();

    return 0;
}

/**
 * @brief: set sharp param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_sharp: sharp param
 */
int ak_isp_vp_set_sharp_attr(AK_ISP_SHARP_ATTR *p_sharp)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->sharp_para),p_sharp,sizeof(AK_ISP_SHARP_ATTR));
	//ak_isp_vo_update_setting();
	isp->linkage_para_update_flag =1;
	
    return 0;
}

/**
 * @brief: get sharp param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_sharp: sharp param
 */
int ak_isp_vp_get_sharp_attr(AK_ISP_SHARP_ATTR* p_sharp)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_sharp,&(isp->sharp_para),sizeof(AK_ISP_SHARP_ATTR));
    
    return 0;
}

/**
 * @brief: set sharp other param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_sharp_ex: sharp other param
 */
int ak_isp_vp_set_sharp_ex_attr(AK_ISP_SHARP_EX_ATTR *p_sharp_ex)
{
    unsigned long cmd;
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->sharp_ex_para),p_sharp_ex, sizeof(AK_ISP_SHARP_EX_ATTR));

    //yuv_reg_28
    cmd = 0;
	cmd = isp->reg_blkaddr3[27];
	CLEAR_BITS(cmd, 24, 6);
	cmd |= (LOW_BITS(p_sharp_ex->mf_HPF[0], 6) << YUV_SHARPEN_M10);
	isp->reg_blkaddr3[27] = cmd;
	
	//yuv_reg_29
	cmd = 0; 
	cmd = (LOW_BITS(p_sharp_ex->mf_HPF[1], 4) << YUV_SHARPEN_M11) 
		| (LOW_BITS(p_sharp_ex->mf_HPF[2], 4) << YUV_SHARPEN_M12) 
		| (LOW_BITS(p_sharp_ex->mf_HPF[3], 4) << YUV_SHARPEN_M13) 
		| (LOW_BITS(p_sharp_ex->mf_HPF[4], 4) << YUV_SHARPEN_M14) 
		| (LOW_BITS(p_sharp_ex->mf_HPF[5], 4) << YUV_SHARPEN_M15) 
		| (LOW_BITS(p_sharp_ex->hf_HPF[0], 6) << YUV_SHARPEN_M20)
		| (LOW_BITS(p_sharp_ex->hf_HPF[1], 4) << YUV_SHARPEN_M21);
	isp->reg_blkaddr3[28]= cmd;

    //yuv_reg_30
    cmd = 0 ;
    cmd = isp->reg_blkaddr3[29];
    CLEAR_BITS(cmd, 0, 4);
    CLEAR_BITS(cmd, 16, 16);
    cmd |= (LOW_BITS(p_sharp_ex->hf_HPF[2], 4) << YUV_SHARPEN_M22)
    	|  (LOW_BITS(p_sharp_ex->sharp_skin_max_th, 8) << YUV_SHARPEN_SKIN_MAX_TH) 
    	|  (LOW_BITS(p_sharp_ex->sharp_skin_min_th, 8) << YUV_SHARPEN_SKIN_MIN_TH);
    isp->reg_blkaddr3[29]= cmd;
    
    //yuv_reg_31
    cmd = 0;
    cmd = (LOW_BITS(p_sharp_ex->sharp_skin_v_max_th, 8) << YUV_SHARPEN_SKIN_V_MAX_TH) 
    	| (LOW_BITS(p_sharp_ex->sharp_skin_v_min_th, 8) << YUV_SHARPEN_SKIN_V_MIN_TH) 
    	| (LOW_BITS(p_sharp_ex->sharp_skin_y_max_th, 8) << YUV_SHARPEN_SKIN_Y_MAX_TH) 
    	| (LOW_BITS(p_sharp_ex->sharp_skin_y_min_th, 8) << YUV_SHARPEN_SKIN_Y_MIN_TH);
    isp->reg_blkaddr3[30]= cmd;

	ak_isp_vo_update_setting();
	
    return 0;
}

/**
 * @brief: get sharp other param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_sharp_ex: sharp other param
 */
int ak_isp_vp_get_sharp_ex_attr(AK_ISP_SHARP_EX_ATTR* p_sharp_ex)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_sharp_ex,&(isp->sharp_ex_para),sizeof(AK_ISP_SHARP_EX_ATTR)); 

    return 0;
}

/**
 * @brief: set false color param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *fcs: false color param
 */
int _set_fcs_attr(AK_ISP_FCS *fcs)
{
    unsigned long cmd;
    isp_dbg("%s enter.\n", __func__);

    //yuv_reg_40
    cmd = 0;
    cmd = (LOW_BITS(fcs->fcs_th, 8) << FCS_TH) 
    	| (LOW_BITS(fcs->fcs_gain_slop, 6) << FCS_SLOP) 
    	| (LOW_BITS(fcs->fcs_uv_nr_th, 10) << FCS_UV_NR_TH);
    //*pReg++ = cmd;
    isp->reg_blkaddr3[39]=  cmd;

    cmd = 0;                                            //reg5
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, FCS_EN, 2);
    cmd |= ((LOW_BITS(fcs->fcs_enable, 1) << FCS_EN)
    	|  (LOW_BITS(fcs->fcs_uv_nr_enable, 1) <<FCS_UV_NR_EN));
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

	ak_isp_vo_update_setting();
	
    return 0;
}

/**
 * @brief: set false color param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *fcs: false color param
 */
int ak_isp_vp_set_fcs_attr(AK_ISP_FCS_ATTR *p_fcs)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->fcs_para),p_fcs,sizeof(AK_ISP_FCS_ATTR));
	//ak_isp_vo_update_setting();
	isp->linkage_para_update_flag =1;

    return 0;
}

/**
 * @brief: get false color param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *fcs: false color param
 */
int ak_isp_vp_get_fcs_attr(AK_ISP_FCS_ATTR *p_fcs)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_fcs,&(isp->fcs_para),sizeof(AK_ISP_FCS_ATTR));

    return 0;
}

/**
 * @brief: set hue param 
 * @author: lz
 * @date: 2016-9-19
 * @param [in] *hue: hue param 
 */
int _set_hue_attr( AK_ISP_HUE *hue)
{
	int i=0;
	unsigned long cmd = 0;
	isp_dbg("%s enter.\n", __func__);
	memcpy(&(isp->hue_info.hue_current), hue, sizeof(AK_ISP_HUE));
		
	for (i=0; i<64; i++)
	{
		cmd = 0;
		cmd = (hue->hue_lut_a[i] & 0xff )      |
			  ((hue->hue_lut_b[i]& 0xff )<< 8) |
			  (hue->hue_lut_s[i] << 16);
		
		isp->cfg_mem_blkaddr6[i] = cmd;
	}

	//enable
	cmd = 0;
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, FCS_HUE_SAT_EN, 1);
    cmd |= (LOW_BITS(hue->hue_sat_en, 1) << FCS_HUE_SAT_EN);     
    isp->reg_blkaddr5[R5_ENABLE] = cmd;
	
	ak_isp_vo_update_setting();

	return 0;
}

/**
 * @brief: set hue param
 * @author: lz
 * @date: 2016-9-12
 * @param [in] *p_hue:hue param
 */
int ak_isp_vp_set_hue_attr(AK_ISP_HUE_ATTR *p_hue)
{
	isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->hue_para),p_hue,sizeof(AK_ISP_HUE_ATTR));
	//ak_isp_vo_update_setting();
	isp->linkage_hue_update_flag =1;

    return 0;
}

/**
 * @brief: gethue param
 * @author: lz
 * @date: 2016-9-12
 * @param [in] *p_hue:hue param
 */
int ak_isp_vp_get_hue_attr(AK_ISP_HUE_ATTR *p_hue)
{
	isp_dbg("%s enter.\n", __func__);
    memcpy(p_hue,&(isp->hue_para),sizeof(AK_ISP_HUE_ATTR));

    return 0;
}

/**
 * @brief: set satruration param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *sat: satruration param
 */
int _set_saturation_attr(AK_ISP_SATURATION *sat)
{
    unsigned long cmd;
	int SE_scale_slop1 = 0;
	int SE_scale_slop2 = 0;
	
	if(sat->SE_th2 == sat->SE_th1)
		SE_scale_slop1 = 128;
	else
		SE_scale_slop1 = 1024/(sat->SE_th2-sat->SE_th1);
	if(sat->SE_th4 == sat->SE_th3)
	  SE_scale_slop2 = 128;
	else
		SE_scale_slop2 = 1024/(sat->SE_th4-sat->SE_th3);
	
    isp_dbg("%s enter.\n", __func__);

    //yuv_reg_7
    cmd = 0;
    cmd = (LOW_BITS(sat->SE_th1, 10) << SE_YTH1) 
    	| (LOW_BITS(sat->SE_th2, 10) << SE_YTH2) 
    	| (LOW_BITS(sat->SE_th3, 10) << SE_YTH3) 
    	| (LOW_BITS(sat->SE_th4, 2) << SE_YTH4_L2);
    //*pReg++ = cmd;
    isp->reg_blkaddr3[6] = cmd;

    //yuv_reg_8
    cmd = 0;
    cmd =(LOW_BITS((sat->SE_th4 >> 2), 8) << SE_YTH4_H_8) 
    	| (LOW_BITS(sat->SE_scale1, 8) << SE_SCALE1) 
    	| (LOW_BITS(sat->SE_scale2, 8) << SE_SCALE2) 
    	| (LOW_BITS(sat->SE_scale3, 8) << SE_SCALE3);
    isp->reg_blkaddr3[7] = cmd;

	//yuv_reg_9
    cmd = 0;
	cmd = isp->reg_blkaddr3[8];
    CLEAR_BITS(cmd, 0, 16);
    cmd |=((LOW_BITS(SE_scale_slop1, 8) << SE_SCALE_SLOP_1)
    	| (LOW_BITS(SE_scale_slop2, 8) << SE_SCALE_SLOP_2));
    isp->reg_blkaddr3[8] = cmd;
	
    //enable
    cmd = 0;                                            //reg5
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, SE_EN, 1);
    cmd |= (LOW_BITS(sat->SE_enable, 1) << SE_EN);
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

	ak_isp_vo_update_setting();
	
    return 0;
}

/**
 * @brief: set satruration param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [in] *p_sat: satruration param
 */
int ak_isp_vp_set_saturation_attr(AK_ISP_SATURATION_ATTR *p_sat)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->saturation_para),p_sat,sizeof(AK_ISP_SATURATION_ATTR));
	//ak_isp_vo_update_setting();
	isp->linkage_para_update_flag =1;

    return 0;
}

/**
 * @brief: get satruration param
 * @author: xiepenghe
 * @date: 2016-5-06
 * @param [out] *p_sat: satruration param
 */
int ak_isp_vp_get_saturation_attr(AK_ISP_SATURATION_ATTR *p_sat)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_sat,&(isp->saturation_para),sizeof(AK_ISP_SATURATION_ATTR));

    return 0;
}

int _set_contrast_attr(AK_ISP_CONTRAST  *p_contrast)
{
    unsigned long cmd ;
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->contrast_setting),p_contrast,sizeof(AK_ISP_CONTRAST));

    //raw_reg_52
    cmd = 0;
    cmd = isp->reg_blkaddr2[51];
    CLEAR_BITS(cmd, 0, 17);
    cmd |= ((LOW_BITS(p_contrast->y_contrast, 8) << RGB2YUV_Y_CONSTRAST) 
    	|  (LOW_BITS(p_contrast->y_shift, 9) << RGB2YUV_Y_SHIFT));
        //(IspC->rgb2yuv_para.yuv_formula << RGB2YUV_FORMULA);
    isp->reg_blkaddr2[51] = cmd;

	ak_isp_vo_update_setting();

    return 0;
}

int ak_isp_vp_set_contrast_attr(AK_ISP_CONTRAST_ATTR  *p_contrast)
{
	isp_dbg("%s enter.\n", __func__);
	memcpy(&(isp->contrast_para),p_contrast,sizeof(AK_ISP_CONTRAST_ATTR));
	isp->linkage_para_update_flag = 1;

	return 0;
}

int ak_isp_vp_get_contrast_attr(AK_ISP_CONTRAST_ATTR  *p_contrast)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_contrast,&(isp->contrast_para),sizeof(AK_ISP_CONTRAST_ATTR));

    return 0;
}

/*
 * Mask areas are encoded differently on the two parts. EV300 gives each of the
 * eight areas two words with 11-bit x and y and its enable at bit 31; EV200
 * has the same eight areas bit-packed as a dense stream across words 0..10,
 * y narrowed to 10 bits, with all eight enables gathered in word 10 bits
 * 24..31. Areas are laid out three to every four words.
 *
 * This is register-block 4's actual layout on EV200.
 */
#ifdef CONFIG_CPU_AK3918EV200
static void _isp_pp_set_mask_area(int area, int sx, int ex, int sy, int ey)
{
	unsigned long *r = isp->reg_blkaddr4;
	int w = (area / 3) * 4;

	switch (area % 3) {
	case 0:
		r[w] = (LOW_BITS(sx, 11) << 0)
		     | (LOW_BITS(ex, 11) << 11)
		     | (LOW_BITS(sy, 10) << 22);
		r[w + 1] = (r[w + 1] & ~0x3ffUL) | LOW_BITS(ey, 10);
		break;
	case 1:
		r[w + 1] = (r[w + 1] & 0x3ffUL)
			 | (LOW_BITS(sx, 11) << 10)
			 | (LOW_BITS(ex, 11) << 21);
		r[w + 2] = (r[w + 2] & ~0xfffffUL)
			 | (LOW_BITS(sy, 10) << 0)
			 | (LOW_BITS(ey, 10) << 10);
		break;
	default:
		r[w + 2] = (r[w + 2] & ~(0x7ffUL << 20))
			 | (LOW_BITS(sx, 11) << 20);
		r[w + 3] = (LOW_BITS(ex, 11) << 0)
			 | (LOW_BITS(sy, 10) << 11)
			 | (LOW_BITS(ey, 10) << 21);
		break;
	}
}

static void _isp_pp_set_mask_enable(int area, int enable)
{
	unsigned long cmd = isp->reg_blkaddr4[10];

	CLEAR_BITS(cmd, 24 + area, 1);
	cmd |= (LOW_BITS(enable, 1) << (24 + area));
	isp->reg_blkaddr4[10] = cmd;
}
#else
static void _isp_pp_set_mask_area(int area, int sx, int ex, int sy, int ey)
{
	unsigned long *r = isp->reg_blkaddr4;

	r[2 * area] = (LOW_BITS(sx, 11) << PP_MASK1_S_XPOS)
		    | (LOW_BITS(ex, 11) << PP_MASK1_E_XPOS);
	r[2 * area + 1] = (r[2 * area + 1] & (1UL << PP_MASK1_EN))
			| (LOW_BITS(sy, 11) << PP_MASK1_S_YPOS)
			| (LOW_BITS(ey, 11) << PP_MASK1_E_YPOS);
}

static void _isp_pp_set_mask_enable(int area, int enable)
{
	unsigned long cmd = isp->reg_blkaddr4[2 * area + 1];

	CLEAR_BITS(cmd, PP_MASK1_EN, 1);
	cmd |= (LOW_BITS(enable, 1) << PP_MASK1_EN);
	isp->reg_blkaddr4[2 * area + 1] = cmd;
}
#endif

int ak_isp_vpp_set_main_chan_mask_area_attr( AK_ISP_MAIN_CHAN_MASK_AREA_ATTR *p_mask)
{
    int i;

    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->main_chan_mask_area_para),p_mask,sizeof(AK_ISP_MAIN_CHAN_MASK_AREA_ATTR));

    for (i = 0; i < 4; i++) {
        AK_ISP_MASK_AREA *m = &isp->main_chan_mask_area_para.mask_area[i];

        _isp_pp_set_mask_area(i, m->start_xpos, m->end_xpos,
                              m->start_ypos, m->end_ypos);
        _isp_pp_set_mask_enable(i, m->enable);
    }

	ak_isp_vo_update_setting();

    return 0;
}

int ak_isp_vpp_get_main_chan_mask_area_attr(AK_ISP_MAIN_CHAN_MASK_AREA_ATTR *p_mask)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_mask,&(isp->main_chan_mask_area_para),sizeof(AK_ISP_MAIN_CHAN_MASK_AREA_ATTR));

    return 0;
}

int ak_isp_vpp_set_sub_chan_mask_area_attr(AK_ISP_SUB_CHAN_MASK_AREA_ATTR *p_mask)
{
    int i;

    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->sub_chan_mask_area_para),p_mask,sizeof(AK_ISP_SUB_CHAN_MASK_AREA_ATTR));

    for (i = 0; i < 4; i++) {
        AK_ISP_MASK_AREA *m = &isp->sub_chan_mask_area_para.mask_area[i];

        _isp_pp_set_mask_area(4 + i, m->start_xpos, m->end_xpos,
                              m->start_ypos, m->end_ypos);
        _isp_pp_set_mask_enable(4 + i, m->enable);
    }

	ak_isp_vo_update_setting();

    return 0;
}

int ak_isp_vpp_get_sub_chan_mask_area_attr(AK_ISP_SUB_CHAN_MASK_AREA_ATTR *p_mask)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_mask,&(isp->sub_chan_mask_area_para),sizeof(AK_ISP_SUB_CHAN_MASK_AREA_ATTR));

    return 0;
}

int ak_isp_vpp_set_mask_area_attr( AK_ISP_MASK_AREA_ATTR *p_mask, MASK_NUM num)
{
    AK_ISP_MASK_AREA *m;
    int area;

    memcpy(&(isp->mask_area_para),p_mask,sizeof(AK_ISP_MASK_AREA_ATTR));
    switch(num)
    {
    case MAIN_CHAN_ONE:
        /* x from mask_area_para, y and enable from sub_chan_mask_area_para -
         * kept verbatim from the SDK source. */
        _isp_pp_set_mask_area(0,
                isp->mask_area_para.mask_area[0].start_xpos,
                isp->mask_area_para.mask_area[0].end_xpos,
                isp->sub_chan_mask_area_para.mask_area[0].start_ypos,
                isp->sub_chan_mask_area_para.mask_area[0].end_ypos);
        _isp_pp_set_mask_enable(0, isp->sub_chan_mask_area_para.mask_area[0].enable);
        break;

    case MAIN_CHAN_TWO:
    case MAIN_CHAN_THREE:
    case MAIN_CHAN_FOURE:
        area = (num == MAIN_CHAN_TWO) ? 1 : (num == MAIN_CHAN_THREE) ? 2 : 3;
        m = &isp->main_chan_mask_area_para.mask_area[area];
        _isp_pp_set_mask_area(area, m->start_xpos, m->end_xpos,
                              m->start_ypos, m->end_ypos);
        _isp_pp_set_mask_enable(area, m->enable);
        break;

    case SUB_CHAN_ONE:
    case SUB_CHAN_TWO:
    case SUB_CHAN_THREE:
    case SUB_CHAN_FOURE:
        area = (num == SUB_CHAN_ONE) ? 0 : (num == SUB_CHAN_TWO) ? 1 :
               (num == SUB_CHAN_THREE) ? 2 : 3;
        m = &isp->sub_chan_mask_area_para.mask_area[area];
        _isp_pp_set_mask_area(4 + area, m->start_xpos, m->end_xpos,
                              m->start_ypos, m->end_ypos);
        _isp_pp_set_mask_enable(4 + area, m->enable);
        break;

    default:
        break;
    }

	ak_isp_vo_update_setting();

    return 0;
}

int ak_isp_vpp_get_mask_area_attr(AK_ISP_MASK_AREA_ATTR *p_mask,  MASK_NUM num)
{
   isp_dbg("%s enter.\n", __func__);
   memcpy(&(isp->mask_area_para),p_mask,sizeof(AK_ISP_MASK_AREA_ATTR));

   return 0;
}

int ak_isp_vpp_set_mask_color( AK_ISP_MASK_COLOR_ATTR *p_mask)
{
    unsigned long cmd = 0;
    
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->mask_color_para),p_mask,sizeof(AK_ISP_MASK_COLOR_ATTR));

    //p_mask.y_mk_color<<
    //pp_reg_29
    cmd = 0;
    cmd = (p_mask->v_mk_color << 0)
    	| (p_mask->u_mk_color << 8)
    	| (p_mask->y_mk_color << 16)
    	| (LOW_BITS(p_mask->mk_alpha, 4) << PP_MASK_ALPHA)
        | (LOW_BITS(p_mask->color_type, 2) << PP_MASK_FORMAT);
    isp->reg_blkaddr4[R4_MASK_COLOR] = cmd;

	ak_isp_vo_update_setting();

    return 0;
}
int ak_isp_vpp_get_mask_color(AK_ISP_MASK_COLOR_ATTR *p_mask)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_mask,&(isp->mask_color_para),sizeof(AK_ISP_MASK_COLOR_ATTR));

   	return 0;
}

int ak_isp_vpp_set_osd_color_table_attr(AK_ISP_OSD_COLOR_TABLE_ATTR *p_isp_color_table)
{
	int i;
	int v_index;
	unsigned long cmd;
    isp_dbg("%s enter.\n", __func__);

	if (!isp)
		return -1;

	//isp->cb.cb_printk("%s %d\n",__func__, __LINE__);
	for (i = 0; i < 16; i++)
	{
		v_index = R4_OSD_COLOR_TABLE + 3 * (i / 4);//lz0499 9_25

		/* set V */
		cmd = isp->reg_blkaddr4[v_index];
		CLEAR_BITS(cmd, (8 * (i % 4)), 8);
		cmd |= (LOW_BITS(*(p_isp_color_table->color_table + i), 8) << (8 * (i % 4)));
    	isp->reg_blkaddr4[v_index] = cmd;

		/* set U */
		cmd = isp->reg_blkaddr4[v_index + 1];
		CLEAR_BITS(cmd, (8 * (i % 4)), 8);
		cmd |= (LOW_BITS((*(p_isp_color_table->color_table + i) >> 8), 8) << (8 * (i % 4)));
    	isp->reg_blkaddr4[v_index + 1] = cmd;

		/* set Y */
		cmd = isp->reg_blkaddr4[v_index + 2];
		CLEAR_BITS(cmd, (8 * (i % 4)), 8);
		cmd |= (LOW_BITS((*(p_isp_color_table->color_table + i) >> 16), 8) << (8 * (i % 4)));
    	isp->reg_blkaddr4[v_index + 2] = cmd;
	}

	ak_isp_vo_update_setting_most();

	return 0;
}

int ak_isp_vpp_mainchn_osdmem_useok(void)
{
	return isp->main_osd_update_flag;
}

int ak_isp_vpp_subchn_osdmem_useok(void)
{
	return isp->sub_osd_update_flag;
}

int ak_isp_vpp_set_main_channel_osd_mem_attr(AK_ISP_OSD_MEM_ATTR *p_mem)
{
	int chn = p_mem->chn;
	T_U32  bytes_osd_bak = isp->bytes_main_osd[chn]; 
	
	isp->bytes_main_osd[chn] = p_mem->size;   
	isp->handle_main_osd[chn] = (T_U32)p_mem->dma_paddr;
	isp->area_main_osd[chn] = p_mem->dma_vaddr;
	if (!isp->area_main_osd[chn]) {
		isp->cb.cb_printk("Failed to alloc dma memory for main osd\n");			
	}
	if (bytes_osd_bak == isp->bytes_main_osd[chn]){
		return 0;
	}

	return 0;
}

int ak_isp_vpp_set_sub_channel_osd_mem_attr(AK_ISP_OSD_MEM_ATTR *p_mem)
{
	int chn = p_mem->chn;
	T_U32   bytes_osd_bak = isp->bytes_sub_osd[chn]; 
	
	isp->bytes_sub_osd[chn] = p_mem->size; 
	isp->handle_sub_osd[chn] = (T_U32)p_mem->dma_paddr;
	isp->area_sub_osd[chn] = p_mem->dma_vaddr;
	if (!isp->area_sub_osd[chn]) {
		isp->cb.cb_printk("Failed to alloc dma memory for sub osd\n");			
	}
	if (bytes_osd_bak == isp->bytes_sub_osd[chn]){
		return 0;
	}

	return 0;
}

static unsigned int isp_main_osd_curr_paddr(int osd_chn)
{
	unsigned int paddr;

	switch (osd_chn) {
		case 0:
			paddr = OSD_DMA_ADDR(isp->reg_blkaddr4[R4_MAIN_OSD]);
			break;
#ifndef CONFIG_CPU_AK3918EV200
		case 1:
			paddr = LOW_BITS(isp->reg_blkaddr4[R4_MAIN_OSD + 3], 30);
			break;
		case 2:
			paddr = LOW_BITS(isp->reg_blkaddr4[R4_MAIN_OSD + 6], 30);
			break;
#endif
		default:
			paddr = 0;
			break;
	}

	return paddr;
}

static unsigned int isp_sub_osd_curr_paddr(int osd_chn)
{
	unsigned int paddr;

	switch (osd_chn) {
		case 0:
			paddr = OSD_DMA_ADDR(isp->reg_blkaddr4[R4_SUB_OSD]);
			break;
#ifndef CONFIG_CPU_AK3918EV200
		case 1:
			paddr = LOW_BITS(isp->reg_blkaddr4[R4_SUB_OSD + 3], 30);
			break;
		case 2:
			paddr = LOW_BITS(isp->reg_blkaddr4[R4_SUB_OSD + 6], 30);
			break;
#endif
		default:
			paddr = 0;
			break;
	}

	return paddr;
}

int ak_isp_vpp_set_main_channel_osd_context_attr(AK_ISP_OSD_CONTEXT_ATTR *p_context)
{
	int chn = p_context->chn;
	AK_ISP_OSD_CONTEXT_ATTR *p_osd_buffer;

	int osd_len;
	T_U8    *p_area_osd;
	T_U32   handle_osd;
	T_U32   bytes_osd;

	if (!isp)
		return -1;

	isp->main_osd_update_flag = 0;

	/* copy all config */
	p_osd_buffer = &isp->main_osd_buffer[chn];
#if 0
	memcpy(p_osd_buffer, p_context, sizeof(AK_ISP_OSD_CONTEXT_ATTR));
#else
	p_area_osd = isp->area_main_osd[chn];
	handle_osd = isp->handle_main_osd[chn];
	bytes_osd = isp->bytes_main_osd[chn] >> 1;
	osd_len = p_context->osd_width * p_context->osd_height / 2;
	osd_len = (osd_len < bytes_osd) ? osd_len:bytes_osd;

	memcpy(p_osd_buffer, p_context, sizeof(AK_ISP_OSD_CONTEXT_ATTR));
	if (OSD_DMA_ADDR(handle_osd) == isp_main_osd_curr_paddr(chn)) {
		memcpy(p_area_osd + bytes_osd, p_context->osd_context_addr, osd_len);
		p_osd_buffer->osd_context_addr = (T_U32 *)(p_area_osd + bytes_osd);
	} else {
		memcpy(p_area_osd, p_context->osd_context_addr, osd_len);
		p_osd_buffer->osd_context_addr = (T_U32 *)(p_area_osd);
	}
#endif
	isp->main_osd_chn_update_flag[chn] = 1;

	isp->main_osd_update_flag = 1;

	return 0;
}

static int ak_isp_vpp_set_main_channel_osd_context_attr_irq(void)
{
	int chn;
	int osd_len;
	unsigned long cmd;
	//T_U8    *p_area_osd;
	T_U32   handle_osd;
	T_U32   bytes_osd;
	AK_ISP_OSD_CONTEXT_ATTR *p_context;

	if (!isp)
		return -1;

	if (!isp->main_osd_update_flag)
		return -1;

	/*osd chn0*/
	chn = 0;
	if (isp->main_osd_chn_update_flag[chn]) {
		p_context = &isp->main_osd_buffer[chn];
		//p_area_osd = isp->area_main_osd[chn];
		handle_osd = isp->handle_main_osd[chn];
		bytes_osd = isp->bytes_main_osd[chn] >> 1;
		osd_len = p_context->osd_width * p_context->osd_height / 2;
		osd_len = (osd_len < bytes_osd) ? osd_len:bytes_osd;
		//isp->cb.cb_printk("OSD CHN0,p_area_osd:%p, p_context->osd_context_addr:%p\n",p_area_osd, p_context->osd_context_addr);
#if 0
		if(p_context->enable)
			memcpy(p_area_osd, p_context->osd_context_addr, osd_len);
#endif

		cmd = isp->reg_blkaddr4[R4_MAIN_OSD];
#if 0
		cmd = LOW_BITS(handle_osd, 30);
#else
		if (OSD_DMA_ADDR(handle_osd) == isp_main_osd_curr_paddr(chn)) {
			cmd = OSD_DMA_ADDR(handle_osd + bytes_osd);
		} else {
			cmd = OSD_DMA_ADDR(handle_osd);
		}
#endif
		isp->reg_blkaddr4[R4_MAIN_OSD] = cmd;

		cmd = isp->reg_blkaddr4[R4_MAIN_OSD + 1];
		cmd = LOW_BITS(p_context->start_xpos, 13);
		cmd += (LOW_BITS(p_context->start_ypos, 13) << 13);
		isp->reg_blkaddr4[R4_MAIN_OSD + 1] = cmd;

		cmd = isp->reg_blkaddr4[R4_MAIN_OSD + 2];
		cmd = LOW_BITS((p_context->start_xpos + p_context->osd_width - 1), 13);
		cmd += (LOW_BITS((p_context->start_ypos + p_context->osd_height - 1), 13) << 13);
		cmd += (LOW_BITS(p_context->alpha, 4) << 28);
		isp->reg_blkaddr4[R4_MAIN_OSD + 2] = cmd;

#ifdef CONFIG_CPU_AK3918EV200
		cmd = isp->reg_blkaddr5[R5_ENABLE];
		CLEAR_BITS(cmd, PP_MAIN_OSD_EN, 1);
		cmd |= (LOW_BITS(p_context->enable, 1) << PP_MAIN_OSD_EN);
		isp->reg_blkaddr5[R5_ENABLE] = cmd;
#else
		cmd = isp->reg_blkaddr4[29];//lz0499
		CLEAR_BITS(cmd, PP_OSD1_ENABLE1, 1);
		cmd |= (p_context->enable<< PP_OSD1_ENABLE1);
		isp->reg_blkaddr4[29] = cmd;
#endif
	}

#ifndef CONFIG_CPU_AK3918EV200
	/*osd chn1*/
	chn = 1;
	if (isp->main_osd_chn_update_flag[chn]) {
		//isp->cb.cb_printk("OSD CHN1\n");
		p_context = &isp->main_osd_buffer[chn];
		//p_area_osd = isp->area_main_osd[chn];
		handle_osd = isp->handle_main_osd[chn];
		bytes_osd = isp->bytes_main_osd[chn] >> 1;
		osd_len = p_context->osd_width * p_context->osd_height / 2;
		osd_len = (osd_len < bytes_osd) ? osd_len:bytes_osd;
#if 0
		if(p_context->enable)
			memcpy(p_area_osd, p_context->osd_context_addr, osd_len);
#endif

		cmd = isp->reg_blkaddr4[32];
#if 0
		cmd = LOW_BITS(handle_osd, 30);
#else
		if (LOW_BITS(handle_osd, 30) == isp_main_osd_curr_paddr(chn)) {
			cmd = LOW_BITS(handle_osd + bytes_osd, 30);
		} else {
			cmd = LOW_BITS(handle_osd, 30);
		}
#endif
		isp->reg_blkaddr4[32] = cmd;

		cmd = isp->reg_blkaddr4[33];
		cmd = LOW_BITS(p_context->start_xpos, 13);
		cmd += (LOW_BITS(p_context->start_ypos, 13) << 13);
		isp->reg_blkaddr4[33] = cmd;

		cmd = isp->reg_blkaddr4[34];
		cmd = LOW_BITS((p_context->start_xpos + p_context->osd_width - 1), 13);
		cmd += (LOW_BITS((p_context->start_ypos + p_context->osd_height - 1), 13) << 13);
		cmd += (LOW_BITS(p_context->alpha, 4) << 28);
		isp->reg_blkaddr4[34] = cmd;


		cmd = isp->reg_blkaddr4[32];//lz0499 
		cmd |= (p_context->enable<< PP_OSD1_ENABLE2);
		isp->reg_blkaddr4[32] = cmd;
	}

	/*osd chn2*/
	chn = 2;
	if (isp->main_osd_chn_update_flag[chn]) {
		//isp->cb.cb_printk("OSD CHN2\n");
		p_context = &isp->main_osd_buffer[chn];
		//p_area_osd = isp->area_main_osd[chn];
		handle_osd = isp->handle_main_osd[chn];
		bytes_osd = isp->bytes_main_osd[chn] >> 1;
		osd_len = p_context->osd_width * p_context->osd_height / 2;
		osd_len = (osd_len < bytes_osd) ? osd_len:bytes_osd;
#if 0
		if(p_context->enable)
			memcpy(p_area_osd, p_context->osd_context_addr, osd_len);
#endif

		cmd = isp->reg_blkaddr4[35];
#if 0
		cmd |= LOW_BITS(handle_osd, 30);
#else
		if (LOW_BITS(handle_osd, 30) == isp_main_osd_curr_paddr(chn)) {
			cmd = LOW_BITS(handle_osd + bytes_osd, 30);
		} else {
			cmd = LOW_BITS(handle_osd, 30);
		}
#endif
		isp->reg_blkaddr4[35] = cmd;

		cmd = isp->reg_blkaddr4[36];
		cmd = LOW_BITS(p_context->start_xpos, 13);
		cmd += (LOW_BITS(p_context->start_ypos, 13) << 13);
		isp->reg_blkaddr4[36] = cmd;

		cmd = isp->reg_blkaddr4[37];
		cmd = LOW_BITS((p_context->start_xpos + p_context->osd_width - 1), 13);
		cmd += (LOW_BITS((p_context->start_ypos + p_context->osd_height - 1), 13) << 13);
		cmd += (LOW_BITS(p_context->alpha, 4) << 28);
		isp->reg_blkaddr4[37] = cmd;


		cmd = isp->reg_blkaddr4[35];//lz0499
		cmd |= (p_context->enable << PP_OSD1_ENABLE3);
		isp->reg_blkaddr4[35] = cmd;
	}
#endif	/* !CONFIG_CPU_AK3918EV200 */

#if 0
	//isp->cb.cb_printk("%s %d area_main_osd:%p, osd_context_addr:%p\n",__func__, __LINE__,isp->area_main_osd,p_context->osd_context_addr);
	p_context = &isp->main_osd_buffer;
	p_area_osd = isp->area_main_osd;
	handle_osd = isp->handle_main_osd;
	bytes_osd = isp->bytes_main_osd;
	osd_len = p_context->osd_width * p_context->osd_height / 2;
	osd_len = (osd_len < bytes_osd) ? osd_len:bytes_osd;
	if(p_context->enable)
		memcpy(p_area_osd, p_context->osd_context_addr, osd_len);

	cmd = isp->reg_blkaddr4[29];
	cmd = LOW_BITS(handle_osd, 30);
	isp->reg_blkaddr4[29] = cmd;

	cmd = isp->reg_blkaddr4[30];
	cmd = LOW_BITS(p_context->start_xpos, 13);
	cmd += (LOW_BITS(p_context->start_ypos, 13) << 13);
	isp->reg_blkaddr4[30] = cmd;

	cmd = isp->reg_blkaddr4[31];
	cmd = LOW_BITS((p_context->start_xpos + p_context->osd_width - 1), 13);
	cmd += (LOW_BITS((p_context->start_ypos + p_context->osd_height - 1), 13) << 13);
	cmd += (LOW_BITS(p_context->alpha, 4) << 28);
	isp->reg_blkaddr4[31] = cmd;


	cmd = isp->reg_blkaddr4[29];//lz0499 
    CLEAR_BITS(cmd, PP_OSD1_ENABLE1, 1);
	cmd |= (p_context->enable<< PP_OSD1_ENABLE1);
	isp->reg_blkaddr4[29] = cmd;
#endif

#if 0

	int osd_len;
	unsigned long cmd;
	T_U8    *p_area_osd;
	T_U32   handle_osd;
	T_U32   bytes_osd;
	AK_ISP_OSD_CONTEXT_ATTR *p_context;

	if (!isp)
		return -1;

	if (!isp->main_osd_update_flag)
		return -1;

	//isp->cb.cb_printk("%s %d area_main_osd:%p, osd_context_addr:%p\n",__func__, __LINE__,isp->area_main_osd,p_context->osd_context_addr);
	p_context = &isp->main_osd_buffer;
	p_area_osd = isp->area_main_osd;
	handle_osd = isp->handle_main_osd;
	bytes_osd = isp->bytes_main_osd;
	osd_len = p_context->osd_width * p_context->osd_height / 2;
	osd_len = (osd_len < bytes_osd) ? osd_len:bytes_osd;
	if(p_context->enable)
		memcpy(p_area_osd, p_context->osd_context_addr, osd_len);

	p_context->start2_xpos = p_context->start1_xpos+osd_len;
	p_context->start2_ypos = p_context->start1_ypos+osd_len;
	p_context->start3_xpos = p_context->start2_xpos+osd_len;
	p_context->start3_ypos = p_context->start2_ypos+osd_len;
	
    cmd = isp->reg_blkaddr4[29];
	cmd |= LOW_BITS(handle_osd, 30);

	cmd = isp->reg_blkaddr4[30];
    cmd = LOW_BITS(p_context->start1_xpos, 13);
	cmd += (LOW_BITS(p_context->start1_ypos, 13) << 13);
	isp->reg_blkaddr4[30] = cmd;

	cmd = isp->reg_blkaddr4[31];
    cmd = LOW_BITS((p_context->start1_xpos + p_context->osd_width - 1), 13);
	cmd += (LOW_BITS((p_context->start1_ypos + p_context->osd_height - 1), 13) << 13);
	cmd += (LOW_BITS(p_context->alpha1, 4) << 28);
	isp->reg_blkaddr4[31] = cmd;

	//isp->cb.cb_printk("%s %d osd width:%d, height:%d,start_xpos:%d,start_ypos:%d,alpha:%d,reg_blkaddr4[26]:0x%x\n",__func__, __LINE__,p_context->osd_width, p_context->osd_height,p_context->start_xpos, p_context->start_ypos,p_context->alpha,isp->reg_blkaddr4[26]);
	//isp->cb.cb_printk("%s %d osd width:%d, height:%d\n",__func__, __LINE__,p_context->osd_width, p_context->osd_height);

    cmd = isp->reg_blkaddr4[29];//lz0499 
    cmd |= (p_context->enable1<< PP_OSD1_ENABLE1);
    isp->reg_blkaddr4[29] = cmd;


    //osd2
	cmd = isp->reg_blkaddr4[32];
	cmd |= LOW_BITS(handle_osd+osd_len, 30);

	cmd = isp->reg_blkaddr4[33];
	cmd = LOW_BITS(p_context->start2_xpos, 13);
	cmd += (LOW_BITS(p_context->start2_ypos, 13) << 13);
	isp->reg_blkaddr4[33] = cmd;

	cmd = isp->reg_blkaddr4[34];
	cmd = LOW_BITS((p_context->start2_xpos + p_context->osd_width - 1), 13);
	cmd += (LOW_BITS((p_context->start2_ypos + p_context->osd_height - 1), 13) << 13);
	cmd += (LOW_BITS(p_context->alpha2, 4) << 28);
	isp->reg_blkaddr4[34] = cmd;
	
	
	cmd = isp->reg_blkaddr4[32];//lz0499 
	cmd |= (p_context->enable2<< PP_OSD1_ENABLE2);
	isp->reg_blkaddr4[32] = cmd;

	//osd3
	cmd = isp->reg_blkaddr4[35];
	cmd |= LOW_BITS(handle_osd+2*osd_len, 30);

	cmd = isp->reg_blkaddr4[36];
	cmd = LOW_BITS(p_context->start3_xpos, 13);
	cmd += (LOW_BITS(p_context->start3_ypos, 13) << 13);
	isp->reg_blkaddr4[36] = cmd;

	cmd = isp->reg_blkaddr4[37];
	cmd = LOW_BITS((p_context->start3_xpos + p_context->osd_width - 1), 13);
	cmd += (LOW_BITS((p_context->start3_ypos + p_context->osd_height - 1), 13) << 13);
	cmd += (LOW_BITS(p_context->alpha3, 4) << 28);
	isp->reg_blkaddr4[37] = cmd;
	
	
	cmd = isp->reg_blkaddr4[35];//lz0499 
	cmd |= (p_context->enable3 << PP_OSD1_ENABLE3);
	isp->reg_blkaddr4[35] = cmd;
#endif
	ak_isp_vo_update_setting_most();

	for (chn = 0; chn < OSD_CHN_NUM; chn++)
		isp->main_osd_chn_update_flag[chn] = 0;

	isp->main_osd_update_flag = 0;

	return 0;
}

int ak_isp_vpp_set_sub_channel_osd_context_attr(AK_ISP_OSD_CONTEXT_ATTR *p_context)
{
	int chn = p_context->chn;
	AK_ISP_OSD_CONTEXT_ATTR *p_osd_buffer;

	int osd_len;
	T_U8    *p_area_osd;
	T_U32   handle_osd;
	T_U32   bytes_osd;

	if (!isp)
		return -1;

	isp->sub_osd_update_flag = 0;
	
	p_osd_buffer = &isp->sub_osd_buffer[chn];
#if 0
	memcpy(p_osd_buffer, p_context, sizeof(AK_ISP_OSD_CONTEXT_ATTR));
#else
	p_area_osd = isp->area_sub_osd[chn];
	handle_osd = isp->handle_sub_osd[chn];
	bytes_osd = isp->bytes_sub_osd[chn] >> 1;
	osd_len = p_context->osd_width * p_context->osd_height / 2;
	osd_len = (osd_len < bytes_osd) ? osd_len:bytes_osd;

	memcpy(p_osd_buffer, p_context, sizeof(AK_ISP_OSD_CONTEXT_ATTR));
	if (OSD_DMA_ADDR(handle_osd) == isp_sub_osd_curr_paddr(chn)) {
		memcpy(p_area_osd + bytes_osd, p_context->osd_context_addr, osd_len);
		p_osd_buffer->osd_context_addr = (T_U32 *)(p_area_osd + bytes_osd);
	} else {
		memcpy(p_area_osd, p_context->osd_context_addr, osd_len);
		p_osd_buffer->osd_context_addr = (T_U32 *)(p_area_osd);
	}
#endif
	isp->sub_osd_chn_update_flag[chn] = 1;

	isp->sub_osd_update_flag = 1;

	return 0;
}

static int ak_isp_vpp_set_sub_channel_osd_context_attr_irq(void)
{
	int chn;
	int osd_len;
	unsigned long cmd;

	//T_U8    *p_area_osd;
	T_U32   handle_osd;
	T_U32   bytes_osd;
	AK_ISP_OSD_CONTEXT_ATTR *p_context;

	if (!isp)
		return -1;

	if (!isp->sub_osd_update_flag)
		return -1;

	/*osd chn0*/
	chn = 0;
	if (isp->sub_osd_chn_update_flag[chn]) {
		p_context = &isp->sub_osd_buffer[chn];
		//p_area_osd = isp->area_sub_osd[chn];
		handle_osd = isp->handle_sub_osd[chn];
		bytes_osd = isp->bytes_sub_osd[chn] >> 1;

		osd_len = p_context->osd_width * p_context->osd_height / 2;
		osd_len = (osd_len <= bytes_osd) ? osd_len:bytes_osd;
#if 0
		if(p_context->enable)
			memcpy(p_area_osd, p_context->osd_context_addr, osd_len);
#endif

		cmd = isp->reg_blkaddr4[R4_SUB_OSD];
#if 0
		cmd = LOW_BITS(handle_osd, 30);
#else
		if (OSD_DMA_ADDR(handle_osd) == isp_sub_osd_curr_paddr(chn)) {
			cmd = OSD_DMA_ADDR(handle_osd + bytes_osd);
		} else {
			cmd = OSD_DMA_ADDR(handle_osd);
		}
#endif
		isp->reg_blkaddr4[R4_SUB_OSD] = cmd;

		cmd = isp->reg_blkaddr4[R4_SUB_OSD + 1];
		cmd = LOW_BITS(p_context->start_xpos, 13);
		cmd += (LOW_BITS(p_context->start_ypos, 13) << 13);
		isp->reg_blkaddr4[R4_SUB_OSD + 1] = cmd;

		cmd = isp->reg_blkaddr4[R4_SUB_OSD + 2];
		cmd = LOW_BITS((p_context->start_xpos + p_context->osd_width - 1), 13);
		cmd += (LOW_BITS((p_context->start_ypos + p_context->osd_height - 1), 13) << 13);
		cmd += (LOW_BITS(p_context->alpha, 4) << 28);
		isp->reg_blkaddr4[R4_SUB_OSD + 2] = cmd;

#ifdef CONFIG_CPU_AK3918EV200
		cmd = isp->reg_blkaddr5[R5_ENABLE];
		CLEAR_BITS(cmd, PP_SUB_OSD_EN, 1);
		cmd |= (LOW_BITS(p_context->enable, 1) << PP_SUB_OSD_EN);
		isp->reg_blkaddr5[R5_ENABLE] = cmd;
#else
		cmd = isp->reg_blkaddr4[38];
		CLEAR_BITS(cmd, PP_OSD2_ENABLE1, 1);
		cmd |= (LOW_BITS(p_context->enable, 1) << PP_OSD2_ENABLE1);
		isp->reg_blkaddr4[38] = cmd;
#endif
	}

#ifndef CONFIG_CPU_AK3918EV200
	/*osd chn1*/
	chn = 1;
	if (isp->sub_osd_chn_update_flag[chn]) {
		p_context = &isp->sub_osd_buffer[chn];
		//p_area_osd = isp->area_sub_osd[chn];
		handle_osd = isp->handle_sub_osd[chn];
		bytes_osd = isp->bytes_sub_osd[chn] >> 1;


		osd_len = p_context->osd_width * p_context->osd_height / 2;
		osd_len = (osd_len <= bytes_osd) ? osd_len:bytes_osd;
#if 0
		if(p_context->enable)
			memcpy(p_area_osd, p_context->osd_context_addr, osd_len);
#endif

		cmd = isp->reg_blkaddr4[41];
#if 0
		cmd |= LOW_BITS(handle_osd, 30);
#else
		if (LOW_BITS(handle_osd, 30) == isp_sub_osd_curr_paddr(chn)) {
			cmd = LOW_BITS(handle_osd + bytes_osd, 30);
		} else {
			cmd = LOW_BITS(handle_osd, 30);
		}
#endif
		isp->reg_blkaddr4[41] = cmd;

		cmd = isp->reg_blkaddr4[42];
		cmd = LOW_BITS(p_context->start_xpos, 13);
		cmd += (LOW_BITS(p_context->start_ypos, 13) << 13);
		isp->reg_blkaddr4[42] = cmd;

		cmd = isp->reg_blkaddr4[43];
		cmd = LOW_BITS((p_context->start_xpos + p_context->osd_width - 1), 13);
		cmd += (LOW_BITS((p_context->start_ypos + p_context->osd_height - 1), 13) << 13);
		cmd += (LOW_BITS(p_context->alpha, 4) << 28);
		isp->reg_blkaddr4[43] = cmd;


		cmd = isp->reg_blkaddr4[41];//lz0499 
		cmd |= (p_context->enable<< PP_OSD2_ENABLE2);
		isp->reg_blkaddr4[41] = cmd;
	}

	/*osd chn2*/
	chn = 2;
	if (isp->sub_osd_chn_update_flag[chn]) {
		p_context = &isp->sub_osd_buffer[chn];
		//p_area_osd = isp->area_sub_osd[chn];
		handle_osd = isp->handle_sub_osd[chn];
		bytes_osd = isp->bytes_sub_osd[chn] >> 1;


		osd_len = p_context->osd_width * p_context->osd_height / 2;
		osd_len = (osd_len <= bytes_osd) ? osd_len:bytes_osd;
#if 0
		if(p_context->enable)
			memcpy(p_area_osd, p_context->osd_context_addr, osd_len);
#endif

		cmd = isp->reg_blkaddr4[44];
#if 0
		cmd |= LOW_BITS(handle_osd, 30);
#else
		if (LOW_BITS(handle_osd, 30) == isp_sub_osd_curr_paddr(chn)) {
			cmd = LOW_BITS(handle_osd + bytes_osd, 30);
		} else {
			cmd = LOW_BITS(handle_osd, 30);
		}
#endif
		isp->reg_blkaddr4[44] = cmd;

		cmd = isp->reg_blkaddr4[45];
		cmd = LOW_BITS(p_context->start_xpos, 13);
		cmd += (LOW_BITS(p_context->start_ypos, 13) << 13);
		isp->reg_blkaddr4[45] = cmd;

		cmd = isp->reg_blkaddr4[46] ;
		cmd = LOW_BITS((p_context->start_xpos + p_context->osd_width - 1), 13);
		cmd += (LOW_BITS((p_context->start_ypos + p_context->osd_height - 1), 13) << 13);
		cmd += (LOW_BITS(p_context->alpha, 4) << 28);
		isp->reg_blkaddr4[46] = cmd;


		cmd = isp->reg_blkaddr4[44];//lz0499
		cmd |= (p_context->enable << PP_OSD2_ENABLE3);
		isp->reg_blkaddr4[44] = cmd;
	}
#endif	/* !CONFIG_CPU_AK3918EV200 */

#if 0
	int osd_len;
	unsigned long cmd;

	T_U8    *p_area_osd;
	T_U32   handle_osd;
	T_U32   bytes_osd;
	AK_ISP_OSD_CONTEXT_ATTR *p_context;

	if (!isp)
		return -1;

	if (!isp->sub_osd_update_flag)
		return -1;

	p_context = &isp->sub_osd_buffer;
	p_area_osd = isp->area_sub_osd;
	handle_osd = isp->handle_sub_osd;
	bytes_osd = isp->bytes_sub_osd;

	//isp->cb.cb_printk("%s %d\n",__func__, __LINE__);
	osd_len = p_context->osd_width * p_context->osd_height / 2;
	osd_len = (osd_len <= bytes_osd) ? osd_len:bytes_osd;
	if(p_context->enable)
		memcpy(p_area_osd, p_context->osd_context_addr, osd_len);

	p_context->start2_xpos = p_context->start1_xpos+osd_len;
	p_context->start2_ypos = p_context->start1_ypos+osd_len;
	p_context->start3_xpos = p_context->start2_xpos+osd_len;
	p_context->start3_ypos = p_context->start2_ypos+osd_len;

    cmd = isp->reg_blkaddr4[38];//lz0499 9_13
	cmd |= LOW_BITS(handle_osd, 30);

	cmd = isp->reg_blkaddr4[39];
    cmd = LOW_BITS(p_context->start1_xpos, 13);
	cmd += (LOW_BITS(p_context->start1_ypos, 13) << 13);
	isp->reg_blkaddr4[39] = cmd;

	cmd = isp->reg_blkaddr4[40];
    cmd = LOW_BITS((p_context->start1_xpos + p_context->osd_width - 1), 13);
	cmd += (LOW_BITS((p_context->start1_ypos + p_context->osd_height - 1), 13) << 13);
	cmd += (LOW_BITS(p_context->alpha1, 4) << 28);
	isp->reg_blkaddr4[40] = cmd;

    cmd = isp->reg_blkaddr4[38];
    cmd |= (LOW_BITS(p_context->enable1, 1) << PP_OSD2_ENABLE1);
    isp->reg_blkaddr4[38] = cmd;

	//isp->cb.cb_printk("%s %d osd width:%d, height:%d,start_xpos:%d,start_ypos:%d,alpha:%d\n",__func__, __LINE__,p_context->osd_width, p_context->osd_height,p_context->start_xpos, p_context->start_ypos,p_context->alpha);
	//isp2_print_reg_table();

	
    //osd2
    cmd = isp->reg_blkaddr4[41];
	cmd |= LOW_BITS(handle_osd+osd_len, 30);

	cmd = isp->reg_blkaddr4[42];
	cmd = LOW_BITS(p_context->start2_xpos, 13);
	cmd += (LOW_BITS(p_context->start2_ypos, 13) << 13);
	isp->reg_blkaddr4[42] = cmd;

	cmd = isp->reg_blkaddr4[43];
	cmd = LOW_BITS((p_context->start2_xpos + p_context->osd_width - 1), 13);
	cmd += (LOW_BITS((p_context->start2_ypos + p_context->osd_height - 1), 13) << 13);
	cmd += (LOW_BITS(p_context->alpha2, 4) << 28);
	isp->reg_blkaddr4[43] = cmd;
	
	
	cmd = isp->reg_blkaddr4[41];//lz0499 
	cmd |= (p_context->enable2<< PP_OSD2_ENABLE2);
	isp->reg_blkaddr4[41] = cmd;

	//osd3
	cmd = isp->reg_blkaddr4[44];
	cmd |= LOW_BITS(handle_osd+2*osd_len, 30);

	cmd = isp->reg_blkaddr4[45];
	cmd = LOW_BITS(p_context->start3_xpos, 13);
	cmd += (LOW_BITS(p_context->start3_ypos, 13) << 13);
	isp->reg_blkaddr4[45] = cmd;

	cmd = isp->reg_blkaddr4[46] ;
	cmd = LOW_BITS((p_context->start3_xpos + p_context->osd_width - 1), 13);
	cmd += (LOW_BITS((p_context->start3_ypos + p_context->osd_height - 1), 13) << 13);
	cmd += (LOW_BITS(p_context->alpha3, 4) << 28);
	isp->reg_blkaddr4[46] = cmd;
	
	
	cmd = isp->reg_blkaddr4[41];//lz0499 
	cmd |= (p_context->enable3 << PP_OSD2_ENABLE3);
	isp->reg_blkaddr4[41] = cmd;
#endif
	ak_isp_vo_update_setting_most();

	for (chn = 0; chn < OSD_CHN_NUM; chn++)
		isp->sub_osd_chn_update_flag[chn] = 0;

	isp->sub_osd_update_flag = 0;

	return 0;
}

static int ak_isp_osd_irq_update(void)
{
	if (!isp)
		return -1;

	ak_isp_vpp_set_main_channel_osd_context_attr_irq();
	ak_isp_vpp_set_sub_channel_osd_context_attr_irq();

	return 0;
}

int ak_isp_vp_set_rgb2yuv_attr(AK_ISP_RGB2YUV_ATTR*p_rgb2yuv_attr)
{
    unsigned long cmd = 0;
    
    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->rgb2yuv_para),p_rgb2yuv_attr,sizeof(AK_ISP_RGB2YUV_ATTR));

    //raw_reg_52
    cmd = 0;
    cmd = isp->reg_blkaddr2[51];
    CLEAR_BITS(cmd, RGB2YUV_FORMULA, 1);
    cmd |= (LOW_BITS(p_rgb2yuv_attr->mode, 1) << RGB2YUV_FORMULA);
    isp->reg_blkaddr2[51] =cmd;

	ak_isp_vo_update_setting();

    return 0;
}

int ak_isp_vp_get_rgb2yuv_attr(AK_ISP_RGB2YUV_ATTR*p_rgb2yuv_attr)
{
    isp_dbg("%s enter.\n", __func__);
    memcpy(p_rgb2yuv_attr,&(isp->rgb2yuv_para),sizeof(AK_ISP_RGB2YUV_ATTR));

    return 0;
}

int ak_isp_vp_set_effect_attr(AK_ISP_EFFECT_ATTR *p_effect_attr)
{
    unsigned long cmd = 0;

    isp_dbg("%s enter.\n", __func__);
    memcpy(&(isp->effect_para),p_effect_attr,sizeof(AK_ISP_EFFECT_ATTR));

    isp->current_y_gain = p_effect_attr->y_a;

    //yuv_reg_41
    cmd = 0;
	cmd = isp->reg_blkaddr3[40];
	CLEAR_BITS(cmd, 0, 25);
    cmd |= ((LOW_BITS(p_effect_attr->y_a, 8) << ENHANCE_Y_A) 
    	|  (LOW_BITS(p_effect_attr->y_b, 8) << ENHANCE_Y_B) 
    	|  (LOW_BITS(p_effect_attr->uv_a, 9) << ENHANCE_UV_A));
    isp->reg_blkaddr3[40] = cmd;

	//yuv_reg_42
    cmd = 0;
	cmd = isp->reg_blkaddr3[41];
	CLEAR_BITS(cmd, 0, 9);
	cmd |= (LOW_BITS(p_effect_attr->uv_b, 9) << ENHANCE_UV_B);
	isp->reg_blkaddr3[41] = cmd;

    //dark margen enable
    cmd = 0;                                            //reg5
    cmd = isp->reg_blkaddr5[R5_ENABLE];
    CLEAR_BITS(cmd, DARK_MARGIN_EN, 1);
    cmd |= (LOW_BITS(p_effect_attr->dark_margin_en, 1) << DARK_MARGIN_EN);
    isp->reg_blkaddr5[R5_ENABLE] = cmd;

    ak_isp_vo_update_setting();

    return 0;
}

int ak_isp_vp_get_effect_attr(AK_ISP_EFFECT_ATTR *p_effect_attr)
{
    isp_dbg("%s enter.\n", __func__);    
    memcpy(p_effect_attr,&(isp->effect_para),sizeof(AK_ISP_EFFECT_ATTR));

    return 0;
}

int ak_isp_set_isp_capturing(int resume)
{
	int cnt;
	unsigned long peri_status;

	if (!isp)
		return -1;

	if (resume) {
		/*1.wait isp finish prev task*/
		cnt = 50;
		while (REG32(isp->base + STATUS_REG_ADDR) & (1 << ISP_UNDER_WORKING)) {
			if (cnt-- > 0) {
				isp->cb.cb_msleep(1);
			} else {
				isp->cb.cb_printk("[%s:%d] reg10 b14 not clear\n", 
						__func__, __LINE__);
				break;
			}
		}

		/*2.check if under-working*/
		if (cnt <= 0)
			isp->cb.cb_printk("[%s:%d] warning: isp under-working\n",
					__func__, __LINE__);

		/*3.set isp enable bit*/
		ak_isp_vi_apply_mode(isp->cur_mode);
		_isp_enable_buffer();
		ak_isp_vo_update_regtable(MODE_ALL);
		peri_status = REG32(isp->base + GEN_CTRL_ADDR);
		SET_BITS(peri_status, ISP_ENABLE, 1);
		REG32(isp->base + GEN_CTRL_ADDR) = peri_status;
	} else {
		isp->pause_flag = 1;

		cnt = 100;
		while (cnt-- > 0) {
			if (isp->pause_flag == 0)
				break;
			isp->cb.cb_msleep(1); //
		}

		if (cnt <= 0)
			isp->cb.cb_printk("[%s:%d] warning: request irq set isp pause fail\n",
					__func__, __LINE__);

		/*3.if none irq coming, pause now*/
		if (REG32(isp->base + GEN_CTRL_ADDR) & (1 << ISP_ENABLE)) {
			/*isp enable bit is not cleared in irq, then clear it now*/
			peri_status = REG32(isp->base + GEN_CTRL_ADDR);
			CLEAR_BITS(peri_status, ISP_ENABLE, 1); //lz0499 9_19
			CLEAR_BITS(peri_status, VIDEO_MODE, 1);
			REG32(isp->base + GEN_CTRL_ADDR) = peri_status;
		}

		/*4.isp enable bit is cleared, the wait for not under-working*/
		cnt = 50;
		while ((REG32(isp->base + STATUS_REG_ADDR) & (1 << ISP_UNDER_WORKING)) &&
				(cnt-- > 0)) {
			isp->cb.cb_msleep(1);
		}

		if (cnt <= 0)
			isp->cb.cb_printk("[%s:%d] warning: isp under working\n",
					__func__, __LINE__);

		/*5.clear global flag*/
		isp->pause_flag = 0;
	}

	return 0;
}

/***************************************************************************/
int  isp2_module_init(AK_ISP_FUNC_CB *cb, AK_ISP_SENSOR_CB *sensor_cb, void *reg_base)
{
    if (!cb || !cb->cb_malloc || !cb->cb_free || !cb->cb_printk || !sensor_cb)
        return -EINVAL;

    isp = cb->cb_malloc(sizeof(AK_ISP_STRUCT));
    if (!isp) {
        cb->cb_printk("Failed to allocate memory for AK_ISP_STRUCT\n");
        return -ENOMEM;
    }

    cb->cb_memcpy(&(isp->cb), cb, sizeof(AK_ISP_FUNC_CB));
    cb->cb_memcpy(&(isp->sensor_cb), sensor_cb, sizeof(AK_ISP_SENSOR_CB));

    isp->base = reg_base;
    //register table total size is (32 * 4 * 6) = 768byte. here is more than real size.
    isp->bytes = STATIC_DEAD_PIXELS_SIZE 
    	+ DMA_CONFIG_REG_SIZE + DMA_STAT_DATA_SIZE;  
    isp->area = cb->cb_dmamalloc(isp->bytes, &(isp->handle));
    if (!isp->area) {
        printk("Failed to allocate memory for register table\n");
        cb->cb_free(isp);
        isp = NULL;
        return -ENOMEM;
    }
#if 0
    isp->bytes_main_osd = 0;  
    isp->area_main_osd = NULL;
    isp->main_osd_buffer.osd_context_addr = NULL;
    isp->bytes_sub_osd = 0;
    isp->area_sub_osd = NULL;
    isp->sub_osd_buffer.osd_context_addr = NULL;
#else
    memset(&isp->bytes_main_osd[0], 0, 
			sizeof(isp->bytes_main_osd) / sizeof(isp->bytes_main_osd[0]));  
    memset(&isp->area_main_osd[0], 0, 
			sizeof(isp->area_main_osd) / sizeof(isp->area_main_osd[0]));  
    memset(&isp->main_osd_buffer[0], 0, 
			sizeof(isp->main_osd_buffer) / sizeof(isp->main_osd_buffer[0]));  
    memset(&isp->bytes_sub_osd[0], 0, 
			sizeof(isp->bytes_sub_osd) / sizeof(isp->bytes_sub_osd[0]));  
    memset(&isp->area_sub_osd[0], 0, 
			sizeof(isp->area_sub_osd) / sizeof(isp->area_sub_osd[0]));  
    memset(&isp->sub_osd_buffer[0], 0, 
			sizeof(isp->sub_osd_buffer) / sizeof(isp->sub_osd_buffer[0]));  
    memset(&isp->main_osd_chn_update_flag[0], 0, 
			sizeof(isp->main_osd_chn_update_flag) / sizeof(isp->main_osd_chn_update_flag[0]));  
    memset(&isp->sub_osd_chn_update_flag[0], 0, 
			sizeof(isp->sub_osd_chn_update_flag) / sizeof(isp->sub_osd_chn_update_flag[0]));  
#endif
	isp->main_osd_update_flag = 0;
	isp->sub_osd_update_flag = 0;
	
    isp_info("isp_module_init: %s\n", ISP_DRV_LIB_VER);
	
    memset(isp->area, 0x0, isp->bytes);

	/* REG_BLOCK_1 */
    isp->reg_blkaddr1     = (unsigned long *)isp->area;
    /* REG_BLOCK_2 */
    isp->reg_blkaddr2     = isp->reg_blkaddr1+(REG_BLK1_SIZE / 4);

    /* MEM_BLOCK_1 */
    isp->cfg_mem_blkaddr1 = isp->reg_blkaddr2 + (REG_BLK2_SIZE / 4);
    /* MEM_BLOCK_2 */
    isp->cfg_mem_blkaddr2 = isp->cfg_mem_blkaddr1 + (MEM_BLK1_SIZE / 4);
    /* MEM_BLOCK_4 */
    isp->cfg_mem_blkaddr4 = isp->cfg_mem_blkaddr2 +(MEM_BLK2_SIZE / 4);
    /* MEM_BLOCK_3 */
    isp->cfg_mem_blkaddr3 = isp->cfg_mem_blkaddr4 +(MEM_BLK4_SIZE / 4);
    /* MEM_BLOCK_5 */
    isp->cfg_mem_blkaddr5 = isp->cfg_mem_blkaddr3 +(MEM_BLK3_SIZE / 4);
    /* MEM_BLOCK_6 */
    isp->cfg_mem_blkaddr6 = isp->cfg_mem_blkaddr5 +(MEM_BLK5_SIZE / 4);

	/* REG_BLOCK_3 */
    isp->reg_blkaddr3 = isp->cfg_mem_blkaddr6 + (MEM_BLK6_SIZE / 4);
    /* REG_BLOCK_4 */
    isp->reg_blkaddr4 = isp->reg_blkaddr3 + (REG_BLK3_SIZE / 4);
    /* REG_BLOCK_5 */
    isp->reg_blkaddr5 = isp->reg_blkaddr4 + (REG_BLK4_SIZE / 4);

    isp->sdpc_area = (unsigned long *)(isp->area + DMA_CONFIG_REG_SIZE);
    isp->sdpc_handle = isp->handle + DMA_CONFIG_REG_SIZE;

    isp->stat_area = (isp->area + DMA_CONFIG_REG_SIZE 
    	+ STATIC_DEAD_PIXELS_SIZE);
    isp->stat_handle = (isp->handle + DMA_CONFIG_REG_SIZE 
    	+ STATIC_DEAD_PIXELS_SIZE);

   	//isp_init_awb_parms(isp);
   	//_cmos_cam_exp_time_init();  //comment by xc
    isp_dbg("Allocate %d bytes for register table\n", isp->bytes);

    return 0;
}

void isp2_module_fini(void)
{
	ak_isp_vi_stop_capturing();
	isp->cb.cb_msleep(500);
	SET_BITS(REG32(0xF0008000 + 0x1C), 19, 1);//disable isp clock
    isp->cb.cb_dmafree(isp->area, isp->bytes, isp->handle);    
    isp->cb.cb_free(isp);
    isp = NULL;
}

/**
 * @brief: isp irq work
 * 
 * @author: xiepenghe
 * @date: 2016-5-06
 **/
int ak_isp_is_ready(void)
{
	return isp != NULL;
}

int ak_isp_irq_work(void)
{	
	int cmd;
	unsigned long peri_status;
	//int i; 
	
	if(!isp)
		return 0;

	if (isp->pause_flag == 1) {
		peri_status = REG32(isp->base + GEN_CTRL_ADDR);
		CLEAR_BITS(peri_status, ISP_ENABLE, 1);
		CLEAR_BITS(peri_status, VIDEO_MODE, 1);
		REG32(isp->base + GEN_CTRL_ADDR) = peri_status;
		isp->pause_flag = 0;
	}

	cmd = REG32(isp->base + 0x10)&0x01;
	if(!cmd)
		return 0;
	isp->frame_cnt++;
	
	//get last frame id
	isp->current_buffer = isp->next_buffer;

	_isp_enable_buffer();
	
	//read frame id
	cmd = REG32(isp->base + (STATUS_REG_ADDR-ISP_BASE_ADDR));
	cmd = (cmd>>MAIN_FRAME_BUF_ID)&0x03;
	if(isp->current_buffer == cmd)
	{
		cmd = REG32(isp->base + ISP_ADDR_EN_ADDR); //lz0499 9_19
		cmd = (cmd>>ISP_ADDR_EN_SHIFT)&0x0f;
		isp->next_buffer= (isp->current_buffer+1)%4;
#ifndef CONFIG_CPU_AK3918EV200

		cmd = low_4bits_revert((unsigned char )cmd);
#endif
		while(((1<<isp->next_buffer)&cmd)==0)
		{
			isp->next_buffer = (isp->next_buffer+1)%4;
		}
	}
	else	//switch to next frame
		isp->next_buffer = cmd;
	
	//AWB handle

	//do 3d nr handle
	cmd = REG32(isp->base + (STATUS_REG_ADDR-ISP_BASE_ADDR));
	cmd &=  0x1<<TNR_DMA_OVERFLOW;
	if(cmd)
	{
		REG32(isp->base + (STATUS_REG_ADDR-ISP_BASE_ADDR)) = 0x1<<TNR_DMA_OVERFLOW;

		cmd = isp->reg_blkaddr5[R5_ENABLE];
		CLEAR_BITS(cmd, TNR_Y_EN, 1);
		CLEAR_BITS(cmd, TNR_UV_EN, 1);
		isp->reg_blkaddr5[R5_ENABLE] = cmd;	
		isp->_3d_nr_status=1;
	}

	ak_isp_osd_irq_update();
	#if 0
	for(i = 0; i < 50;i++)
		isp2_print_reg_table();
	#endif
	ak_isp_vo_update_regtable(isp->reg_update_flag);
	isp->reg_update_flag = MODE_LEAST;

	_get_3dnr_stat();
	
	return 1;
}

#define MAX_SENSORS	32
static void *sensors_info[MAX_SENSORS] = {0};
int ak_isp_register_sensor(void *sensor_info)
{
	int i;
	void **infos = &sensors_info[0];

	for (i = 0; i < MAX_SENSORS; i++) {
		if (!infos[i]) {
			infos[i] = sensor_info;
			return 0;
		}
	}

	return -1;
}

void *ak_isp_get_sensor(int *index)
{
	int i;
	void **infos = &sensors_info[0];

	for (i = *index; i < MAX_SENSORS; i++) {
		if (infos[i]) {
			*index = i;
			return infos[i];
		}
	}

	return NULL;
}

void ak_isp_remove_all_sensors(void)
{
	int i;
	void **infos = &sensors_info[0];

	for (i = 0; i < MAX_SENSORS; i++)
		infos[i] = NULL;
}

int ak_isp_set_td()
{
	isp->_3dnr_reset = 1;
	/*notice 3dnr need update*/
	isp->linkage_para_update_flag = 1;

	pr_debug("ak39_isp2: 3dnr reset armed\n");
	return 0;
}

int ak_isp_reload_td()
{
	isp->_3dnr_reset = 0;
	/*notice 3dnr need update*/
	isp->linkage_para_update_flag = 1;

	pr_debug("ak39_isp2: 3dnr reset released\n");
	return 0;
}

enum scene ak_isp_get_scene(void)
{
	int iso;
	enum scene s;

	iso = _get_iso();
	if (iso == 0)
		s = SCENE_OUTDOOR;
	else
		s = SCENE_INDOOR;

	return s;
}

int ak_isp_get_iso(void)
{
	return _get_iso();
}

int ak_isp_get_yuvaddr_and_mdinfo(enum buffer_id  id, void **yuv, void **mdinfo)
{
	isp_dbg("%s: id=%d\n", __func__, id);
	int sub_len = 0;
        
	/* get yuv tail addr */ //lz0499 9_6 chanel3
	switch(id)
	{
    case BUFFER_ONE:
		sub_len = (isp->reg_blkaddr5[30] - isp->reg_blkaddr5[29]) * 2;
		*yuv = (void *)(((T_U8 *)isp->reg_blkaddr5[29]) + sub_len);
        break;
    case BUFFER_TWO:
		sub_len = (isp->reg_blkaddr1[46] - isp->reg_blkaddr1[43]) * 2;
		*yuv = (void *)(((T_U8 *)isp->reg_blkaddr1[43]) + sub_len);
        break;
    case BUFFER_THREE:
		sub_len = (isp->reg_blkaddr1[47] - isp->reg_blkaddr1[44]) * 2;
		*yuv = (void *)(((T_U8 *)isp->reg_blkaddr1[44]) + sub_len);
        break;
    case BUFFER_FOUR:
		sub_len = (isp->reg_blkaddr1[48] - isp->reg_blkaddr1[45]) * 2;
		*yuv = (void *)(((T_U8 *)isp->reg_blkaddr1[45]) + sub_len);
        break;
    default:
		*yuv = NULL;
        break;
    }

	/* get md info addr */
	*mdinfo = isp->_3d_nr_stat_para.MD_stat;

	return 0;
}
