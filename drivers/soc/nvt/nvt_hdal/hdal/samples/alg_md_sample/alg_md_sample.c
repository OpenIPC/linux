#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include <kwrap/examsys.h>
#include "math.h"
#include "vendor_md.h"

#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/task.h>
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    vos_task_delay_ms(1000*x)
#define msleep(x)   vos_task_delay_ms(x)
#define usleep(x)   vos_task_delay_us(x)
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(alg_md_sample, argc, argv)
#endif

//#define IMG_WIDTH         640//160
//#define IMG_HEIGHT        480//120
//#define IMG_BUF_SIZE      (IMG_WIDTH * IMG_HEIGHT)
#define MDBC_ALIGN(a, b) (((a) + ((b) - 1)) / (b) * (b))
#define OUTPUT_BMP 		0
#define DEBUG_FILE		1
#define SHARE_MEM_NUM	8
UINT32 IMG_WIDTH;
UINT32 IMG_HEIGHT;
UINT32 IMG_BUF_SIZE;

typedef struct _MEM_RANGE {
	UINT32               va;        ///< Memory buffer starting address
	UINT32               addr;      ///< Memory buffer starting address
	UINT32               size;      ///< Memory buffer size
	HD_COMMON_MEM_VB_BLK blk;
} MEM_RANGE, *PMEM_RANGE;

typedef enum {
	LOW_SENSI         = 0,
	MED_SENSI         = 1,
	HIGH_SENSI        = 2,
	DEFAULT_SENSI     = 3,
	ENUM_DUMMY4WORD(MDBC_PARA_SENSI)
} MDBC_PARA_SENSI;

#pragma pack(2)
struct BmpFileHeader {
    UINT16 bfType;
    UINT32 bfSize;
    UINT16 bfReserved1;
    UINT16 bfReserved2;
    UINT32 bfOffBits;
};
struct BmpInfoHeader {
    UINT32 biSize;
    UINT32 biWidth;
    UINT32 biHeight;
    UINT16 biPlanes; // 1=defeaul, 0=custom
    UINT16 biBitCount;
    UINT32 biCompression;
    UINT32 biSizeImage;
    UINT32 biXPelsPerMeter; // 72dpi=2835, 96dpi=3780
    UINT32 biYPelsPerMeter; // 120dpi=4724, 300dpi=11811
    UINT32 biClrUsed;
    UINT32 biClrImportant;
};
#pragma pack()

UINT32 g_md_sensi = DEFAULT_SENSI;

static INT32 share_memory_init(MEM_RANGE *p_share_mem)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT8 i;
	UINT32 pa, va;
	UINT32 blk_size = IMG_BUF_SIZE;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT ret = HD_OK;

	for(i=0;i<SHARE_MEM_NUM;i++){
		p_share_mem[i].addr = 0x00;
		p_share_mem[i].va   = 0x00;
		p_share_mem[i].size = 0x00;
		p_share_mem[i].blk  = HD_COMMON_MEM_VB_INVALID_BLK;
	}
	for(i=0;i<SHARE_MEM_NUM;i++){
		if(i==0)blk_size = IMG_BUF_SIZE;
		else if(i==1 || i==2)blk_size = IMG_BUF_SIZE/2;
		else if(i==3)blk_size = IMG_BUF_SIZE*40;
		else if(i==4)blk_size = IMG_BUF_SIZE*6;
		else if(i==5)blk_size = ((IMG_BUF_SIZE+15)/16)*16*12;
		else if(i==6)blk_size = ((IMG_BUF_SIZE+7)/8);
		else if(i==7)blk_size = IMG_BUF_SIZE; // result transform
		blk = hd_common_mem_get_block(HD_COMMON_MEM_GLOBAL_MD_POOL, blk_size, ddr_id);
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("err:get block fail\r\n", blk);
			ret =  HD_ERR_NG;
			return ret;
		}
		pa = hd_common_mem_blk2pa(blk);
		if (pa == 0) {
			printf("err:blk2pa fail, blk = 0x%x\r\n", blk);
			goto blk2pa_err;
		}
		printf("pa = 0x%x\r\n", pa);
		if (pa > 0) {
			va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size);
			if (va == 0) {
				goto map_err;
			}
		}
		p_share_mem[i].addr = pa;
		p_share_mem[i].va   = va;
		p_share_mem[i].size = blk_size;
		p_share_mem[i].blk  = blk;
		printf("share_mem[%d] pa = 0x%x, va=0x%x, size =0x%x\r\n",i, p_share_mem[i].addr, p_share_mem[i].va, p_share_mem[i].size);
	}
	return ret;
blk2pa_err:
map_err:
	for (; i > 0 ;) {
		i -= 1;
		ret = hd_common_mem_release_block(p_share_mem[i].blk);
		if (HD_OK != ret) {
			printf("err:release blk fail %d\r\n", ret);
			ret =  HD_ERR_NG;
			return ret;
		}
	}
	return ret;
}

static INT32 share_memory_exit(MEM_RANGE *p_share_mem)
{
	UINT8 i;
	for(i=0;i<SHARE_MEM_NUM;i++){
		if (p_share_mem[i].va != 0) {
			hd_common_mem_munmap((void *)p_share_mem[i].va, p_share_mem[i].size);
		}
		if (p_share_mem[i].blk != HD_COMMON_MEM_VB_INVALID_BLK) {
			hd_common_mem_release_block(p_share_mem[i].blk);
		}
		p_share_mem[i].addr = 0x00;
		p_share_mem[i].va   = 0x00;
		p_share_mem[i].size = 0x00;
		p_share_mem[i].blk  = HD_COMMON_MEM_VB_INVALID_BLK;
	}
	return HD_OK;
}

static HD_RESULT mem_init(void)
{
	HD_RESULT ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	mem_cfg.pool_info[0].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[0].blk_size = IMG_BUF_SIZE;
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	mem_cfg.pool_info[1].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[1].blk_size = IMG_BUF_SIZE/2;
	mem_cfg.pool_info[1].blk_cnt = 2;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	mem_cfg.pool_info[2].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[2].blk_size = IMG_BUF_SIZE*40;
	mem_cfg.pool_info[2].blk_cnt = 1;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

	mem_cfg.pool_info[3].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[3].blk_size = IMG_BUF_SIZE*6;
	mem_cfg.pool_info[3].blk_cnt = 1;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;

	mem_cfg.pool_info[4].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[4].blk_size = ((IMG_BUF_SIZE+15)/16)*16*12;
	mem_cfg.pool_info[4].blk_cnt = 1;
	mem_cfg.pool_info[4].ddr_id = DDR_ID0;

	mem_cfg.pool_info[5].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[5].blk_size = ((IMG_BUF_SIZE+7)/8);
	mem_cfg.pool_info[5].blk_cnt = 1;
	mem_cfg.pool_info[5].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("err:hd_common_mem_init err: %d\r\n", ret);
	}
	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

static void bc_reorgS1(UINT8* inputS, UINT8* outputS,UINT32 width, UINT32 height)
{
	UINT32 i,j,count,size;
	count=0;
	size = width*height;
	for(j = 0; j < MDBC_ALIGN(size,8)/8; j++) {
		UINT8 c = inputS[j];
        for(i = 0; i < 8; i++) {
			if(count<size)
			{
				outputS[count] = c & 0x1;
				c = c>>1;
				count++;
			}
        }
    }
}
#if OUTPUT_BMP
static void bc_writebmpfile(char* name, UINT8* raw_img,
    int width, int height, UINT16 bits)
{
    if(!(name && raw_img)) {
        printf("Error bmpWrite.");
        return;
    }
	int i,j;//,length;
    // FileHeader
    struct BmpFileHeader file_h = {
        .bfType=0x4d42,
        .bfSize=0,
        .bfReserved1=0,
        .bfReserved2=0,
        .bfOffBits=54,
    };
    file_h.bfSize = file_h.bfOffBits + width*height * bits/8;
    if(bits==8) {file_h.bfSize += 1024, file_h.bfOffBits += 1024;}
    // BmpInfoHeader
    struct BmpInfoHeader info_h = {
        .biSize=40,
        .biWidth=0,
        .biHeight=0,
        .biPlanes=1,
        .biBitCount=0,
        .biCompression=0,
        .biSizeImage=0,
        .biXPelsPerMeter=0,
        .biYPelsPerMeter=0,
        .biClrUsed=0,
        .biClrImportant=0,
    };
    info_h.biWidth = width;
    info_h.biHeight = height;
    info_h.biBitCount = bits;
    info_h.biSizeImage = width*height * bits/8;
    if(bits == 8) {
        info_h.biClrUsed=256;
        info_h.biClrImportant=256;
    }
    // Write Header
	/*
	INT32 FileHandleStatus = 0;
	FST_FILE filehdl = NULL;
	filehdl = FileSys_OpenFile(name, FST_OPEN_WRITE | FST_CREATE_ALWAYS);
	if (!filehdl) {
		emu_msg(("^ROpen file fail - %s...\r\n", name));
	}
	length = sizeof(file_h);
	FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&file_h, &length, 0, NULL);
	length = sizeof(info_h);
	FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&info_h, &length, 0, NULL);
	*/
    FILE *pFile = fopen(name,"wb+");
    if(!pFile) {
        printf("Error opening file.");
        return;
    }
    fwrite((char*)&file_h, sizeof(char), sizeof(file_h), pFile);
    fwrite((char*)&info_h, sizeof(char), sizeof(info_h), pFile);

	//length = 1;
	// Write colormap
	//printf("Write colormap...\r\n");
    if(bits == 8) {
        for(i = 0; i < 256; ++i) {
            UINT8 c = i;
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
            fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            fwrite("", sizeof(char), sizeof(UINT8), pFile);
        }
    }
    // Write raw img
	//printf("Write raw img...\r\n");
    UINT8 alig = ((width*bits/8)*3) % 4;
    for(j = height-1; j >= 0; --j) {
		//printf("j : %d\r\n",j);
        for(i = 0; i < width; ++i) {
			UINT8 c;
            if(bits == 24) {
				//c = raw_img[(j*width+i)*3 + 2];
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
				//c = raw_img[(j*width+i)*3 + 1];
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
				//c = raw_img[(j*width+i)*3 + 0];
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
                fwrite((char*)&raw_img[(j*width+i)*3 + 2], sizeof(char), sizeof(UINT8), pFile);
                fwrite((char*)&raw_img[(j*width+i)*3 + 1], sizeof(char), sizeof(UINT8), pFile);
                fwrite((char*)&raw_img[(j*width+i)*3 + 0], sizeof(char), sizeof(UINT8), pFile);
            } else if(bits == 8) {
				if(raw_img[j*width+i]==1){ c = 255;}
				else if(raw_img[j*width+i]==0){ c = 0;}
				else{
					printf("raw_img[j*width+i] = %d, (%d,%d)\n",raw_img[j*width+i],i,j);
					c = 0;
				}
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
                fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            }
        }
        // 4byte align
        for(i = 0; i < alig; ++i) {
			//UINT8 c = 0;
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
            fwrite("", sizeof(char), sizeof(UINT8), pFile);
        }
    }
	//FileHandleStatus = FileSys_CloseFile(filehdl);
	//if (FileHandleStatus != FST_STA_OK) {
	//	emu_msg(("^RClose file fail - %s...\r\n", name));
	//}
    fclose(pFile);
}
#endif
static UINT32 md_load_file(CHAR *p_filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 addr = va;
	//printf("addr = %08x\r\n", (int)addr);

	fd = fopen(p_filename, "rb");
	if (!fd) {
		printf("cannot read %s\r\n", p_filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	return read_size;
}

static void md_set_para_default_sensitivity(VENDOR_MD_PARAM *mdbc_parm)
{
	mdbc_parm->MdmatchPara.lbsp_th    = 0x0;
	mdbc_parm->MdmatchPara.d_colour   = 0x6;
	mdbc_parm->MdmatchPara.r_colour   = 0x1e;
	mdbc_parm->MdmatchPara.d_lbsp     = 0x3;
	mdbc_parm->MdmatchPara.r_lbsp     = 0x5;
	mdbc_parm->MdmatchPara.model_num  = 0x8;
	mdbc_parm->MdmatchPara.t_alpha    = 0x33;
	mdbc_parm->MdmatchPara.dw_shift   = 0x4;
	mdbc_parm->MdmatchPara.dlast_alpha= 0x28;
	mdbc_parm->MdmatchPara.min_match  = 2;
	mdbc_parm->MdmatchPara.dlt_alpha  = 0xa;
	mdbc_parm->MdmatchPara.dst_alpha  = 0x28;
	mdbc_parm->MdmatchPara.uv_thres   = 0x6;
	mdbc_parm->MdmatchPara.s_alpha    = 0x28;
	mdbc_parm->MdmatchPara.dbg_lumDiff= 0x0;
	mdbc_parm->MdmatchPara.dbg_lumDiff_en = 0x0;

	mdbc_parm->MorPara.mor_th0 	  = 0x4;
	mdbc_parm->MorPara.mor_th1    = 0x0;
	mdbc_parm->MorPara.mor_th2    = 0x8;
	mdbc_parm->MorPara.mor_th3    = 0x0;
	mdbc_parm->MorPara.mor_th_dil = 0x0;
	mdbc_parm->MorPara.mor_sel0   = 0x1;
	mdbc_parm->MorPara.mor_sel1   = 0x1;
	mdbc_parm->MorPara.mor_sel2   = 0x1;
	mdbc_parm->MorPara.mor_sel3   = 0x0;

	mdbc_parm->UpdPara.minT           = 0x2;
	mdbc_parm->UpdPara.maxT           = 0xff;
	mdbc_parm->UpdPara.maxFgFrm       = 0xff;
	mdbc_parm->UpdPara.deghost_dth    = 0x5;
	mdbc_parm->UpdPara.deghost_sth    = 0xfa;
	mdbc_parm->UpdPara.stable_frm     = 0x78;
	mdbc_parm->UpdPara.update_dyn     = 0x80;
	mdbc_parm->UpdPara.va_distth      = 32;
	mdbc_parm->UpdPara.t_distth       = 24;
	mdbc_parm->UpdPara.dbg_frmID      = 0x0;
	mdbc_parm->UpdPara.dbg_frmID_en   = 0x0;
	mdbc_parm->UpdPara.dbg_rnd        = 0x0;
	mdbc_parm->UpdPara.dbg_rnd_en     = 0x0;
}

static void md_set_para_high_sensitivity(VENDOR_MD_PARAM *mdbc_parm)
{
	mdbc_parm->MdmatchPara.lbsp_th    = 0x0;
	mdbc_parm->MdmatchPara.d_colour   = 10;
	mdbc_parm->MdmatchPara.r_colour   = 0x1e;
	mdbc_parm->MdmatchPara.d_lbsp     = 4;
	mdbc_parm->MdmatchPara.r_lbsp     = 8;
	mdbc_parm->MdmatchPara.model_num  = 0x8;
	mdbc_parm->MdmatchPara.t_alpha    = 0x33;
	mdbc_parm->MdmatchPara.dw_shift   = 0x4;
	mdbc_parm->MdmatchPara.dlast_alpha= 0x28;
	mdbc_parm->MdmatchPara.min_match  = 2;
	mdbc_parm->MdmatchPara.dlt_alpha  = 0xa;
	mdbc_parm->MdmatchPara.dst_alpha  = 0x28;
	mdbc_parm->MdmatchPara.uv_thres   = 20;
	mdbc_parm->MdmatchPara.s_alpha    = 0x28;
	mdbc_parm->MdmatchPara.dbg_lumDiff= 0x0;
	mdbc_parm->MdmatchPara.dbg_lumDiff_en = 0x0;

	mdbc_parm->MorPara.mor_th0 	  = 0x0;
	mdbc_parm->MorPara.mor_th1    = 0x0;
	mdbc_parm->MorPara.mor_th2    = 0x0;
	mdbc_parm->MorPara.mor_th3    = 0x0;
	mdbc_parm->MorPara.mor_th_dil = 0x0;
	mdbc_parm->MorPara.mor_sel0   = 0x0;
	mdbc_parm->MorPara.mor_sel1   = 0x0;
	mdbc_parm->MorPara.mor_sel2   = 0x0;
	mdbc_parm->MorPara.mor_sel3   = 0x0;

	mdbc_parm->UpdPara.minT           = 0x4;
	mdbc_parm->UpdPara.maxT           = 0x40;
	mdbc_parm->UpdPara.maxFgFrm       = 0x80;
	mdbc_parm->UpdPara.deghost_dth    = 50;
	mdbc_parm->UpdPara.deghost_sth    = 205;
	mdbc_parm->UpdPara.stable_frm     = 0x78;
	mdbc_parm->UpdPara.update_dyn     = 0x80;
	mdbc_parm->UpdPara.va_distth      = 32;
	mdbc_parm->UpdPara.t_distth       = 24;
	mdbc_parm->UpdPara.dbg_frmID      = 0x0;
	mdbc_parm->UpdPara.dbg_frmID_en   = 0x0;
	mdbc_parm->UpdPara.dbg_rnd        = 0x0;
	mdbc_parm->UpdPara.dbg_rnd_en     = 0x0;
}

static void md_set_para_medium_sensitivity(VENDOR_MD_PARAM *mdbc_parm)
{
	mdbc_parm->MdmatchPara.lbsp_th    = 0x0;
	mdbc_parm->MdmatchPara.d_colour   = 15;
	mdbc_parm->MdmatchPara.r_colour   = 0x1e;
	mdbc_parm->MdmatchPara.d_lbsp     = 5;
	mdbc_parm->MdmatchPara.r_lbsp     = 10;
	mdbc_parm->MdmatchPara.model_num  = 0x8;
	mdbc_parm->MdmatchPara.t_alpha    = 25;
	mdbc_parm->MdmatchPara.dw_shift   = 0x4;
	mdbc_parm->MdmatchPara.dlast_alpha= 100;
	mdbc_parm->MdmatchPara.min_match  = 2;
	mdbc_parm->MdmatchPara.dlt_alpha  = 0xa;
	mdbc_parm->MdmatchPara.dst_alpha  = 0x28;
	mdbc_parm->MdmatchPara.uv_thres   = 20;
	mdbc_parm->MdmatchPara.s_alpha    = 100;
	mdbc_parm->MdmatchPara.dbg_lumDiff= 0x0;
	mdbc_parm->MdmatchPara.dbg_lumDiff_en = 0x0;

	mdbc_parm->MorPara.mor_th0 	  = 0x0;
	mdbc_parm->MorPara.mor_th1    = 0x0;
	mdbc_parm->MorPara.mor_th2    = 0x0;
	mdbc_parm->MorPara.mor_th3    = 0x0;
	mdbc_parm->MorPara.mor_th_dil = 0x0;
	mdbc_parm->MorPara.mor_sel0   = 0x0;
	mdbc_parm->MorPara.mor_sel1   = 0x0;
	mdbc_parm->MorPara.mor_sel2   = 0x0;
	mdbc_parm->MorPara.mor_sel3   = 0x0;

	mdbc_parm->UpdPara.minT           = 0x4;
	mdbc_parm->UpdPara.maxT           = 0x40;
	mdbc_parm->UpdPara.maxFgFrm       = 0x80;
	mdbc_parm->UpdPara.deghost_dth    = 50;
	mdbc_parm->UpdPara.deghost_sth    = 205;
	mdbc_parm->UpdPara.stable_frm     = 0x78;
	mdbc_parm->UpdPara.update_dyn     = 0x80;
	mdbc_parm->UpdPara.va_distth      = 32;
	mdbc_parm->UpdPara.t_distth       = 24;
	mdbc_parm->UpdPara.dbg_frmID      = 0x0;
	mdbc_parm->UpdPara.dbg_frmID_en   = 0x0;
	mdbc_parm->UpdPara.dbg_rnd        = 0x0;
	mdbc_parm->UpdPara.dbg_rnd_en     = 0x0;
}

static void md_set_para_low_sensitivity(VENDOR_MD_PARAM *mdbc_parm)
{
	mdbc_parm->MdmatchPara.lbsp_th    = 0x0;
	mdbc_parm->MdmatchPara.d_colour   = 15;
	mdbc_parm->MdmatchPara.r_colour   = 0x1e;
	mdbc_parm->MdmatchPara.d_lbsp     = 5;
	mdbc_parm->MdmatchPara.r_lbsp     = 10;
	mdbc_parm->MdmatchPara.model_num  = 0x8;
	mdbc_parm->MdmatchPara.t_alpha    = 25;
	mdbc_parm->MdmatchPara.dw_shift   = 0x4;
	mdbc_parm->MdmatchPara.dlast_alpha= 100;
	mdbc_parm->MdmatchPara.min_match  = 1;
	mdbc_parm->MdmatchPara.dlt_alpha  = 0xa;
	mdbc_parm->MdmatchPara.dst_alpha  = 0x28;
	mdbc_parm->MdmatchPara.uv_thres   = 20;
	mdbc_parm->MdmatchPara.s_alpha    = 100;
	mdbc_parm->MdmatchPara.dbg_lumDiff= 0x0;
	mdbc_parm->MdmatchPara.dbg_lumDiff_en = 0x0;

	mdbc_parm->MorPara.mor_th0 	  = 0x0;
	mdbc_parm->MorPara.mor_th1    = 0x0;
	mdbc_parm->MorPara.mor_th2    = 0x0;
	mdbc_parm->MorPara.mor_th3    = 0x0;
	mdbc_parm->MorPara.mor_th_dil = 0x0;
	mdbc_parm->MorPara.mor_sel0   = 0x0;
	mdbc_parm->MorPara.mor_sel1   = 0x0;
	mdbc_parm->MorPara.mor_sel2   = 0x0;
	mdbc_parm->MorPara.mor_sel3   = 0x0;

	mdbc_parm->UpdPara.minT           = 0x4;
	mdbc_parm->UpdPara.maxT           = 0x40;
	mdbc_parm->UpdPara.maxFgFrm       = 0x80;
	mdbc_parm->UpdPara.deghost_dth    = 50;
	mdbc_parm->UpdPara.deghost_sth    = 205;
	mdbc_parm->UpdPara.stable_frm     = 0x78;
	mdbc_parm->UpdPara.update_dyn     = 0x80;
	mdbc_parm->UpdPara.va_distth      = 32;
	mdbc_parm->UpdPara.t_distth       = 24;
	mdbc_parm->UpdPara.dbg_frmID      = 0x0;
	mdbc_parm->UpdPara.dbg_frmID_en   = 0x0;
	mdbc_parm->UpdPara.dbg_rnd        = 0x0;
	mdbc_parm->UpdPara.dbg_rnd_en     = 0x0;
}

static HD_RESULT md_set_para(MEM_RANGE *p_share_mem, UINT32 mode, UINT32 sensi)
{
	VENDOR_MD_PARAM 		   mdbc_parm;
	HD_RESULT ret = HD_OK;

	mdbc_parm.mode = mode;
	mdbc_parm.controlEn.update_nei_en = 1;
	mdbc_parm.controlEn.deghost_en    = 1;
	mdbc_parm.controlEn.roi_en0       = 0;
	mdbc_parm.controlEn.roi_en1       = 0;
	mdbc_parm.controlEn.roi_en2       = 0;
	mdbc_parm.controlEn.roi_en3       = 0;
	mdbc_parm.controlEn.roi_en4       = 0;
	mdbc_parm.controlEn.roi_en5       = 0;
	mdbc_parm.controlEn.roi_en6       = 0;
	mdbc_parm.controlEn.roi_en7       = 0;
	mdbc_parm.controlEn.chksum_en     = 0;
	mdbc_parm.controlEn.bgmw_save_bw_en = 1;
	mdbc_parm.controlEn.bc_y_only_en 	= 0;
#if 0
	if(ping_pong_id == 0)
	{
		mdbc_parm.InInfo.uiInAddr0 = p_share_mem[0].addr;
		mdbc_parm.InInfo.uiInAddr1 = p_share_mem[1].addr;
		mdbc_parm.InInfo.uiInAddr2 = p_share_mem[2].addr;
		mdbc_parm.InInfo.uiInAddr3 = p_share_mem[3].addr;
		mdbc_parm.InInfo.uiInAddr4 = p_share_mem[4].addr;
		mdbc_parm.InInfo.uiInAddr5 = p_share_mem[5].addr;
		mdbc_parm.OutInfo.uiOutAddr0 = p_share_mem[6].addr;
		mdbc_parm.OutInfo.uiOutAddr1 = p_share_mem[7].addr;
		mdbc_parm.OutInfo.uiOutAddr2 = p_share_mem[8].addr;
		mdbc_parm.OutInfo.uiOutAddr3 = p_share_mem[9].addr;
	} else {
		mdbc_parm.InInfo.uiInAddr0 = p_share_mem[0].addr;
		mdbc_parm.InInfo.uiInAddr1 = p_share_mem[1].addr;
		mdbc_parm.InInfo.uiInAddr2 = p_share_mem[2].addr;
		mdbc_parm.InInfo.uiInAddr3 = p_share_mem[7].addr;
		mdbc_parm.InInfo.uiInAddr4 = p_share_mem[8].addr;
		mdbc_parm.InInfo.uiInAddr5 = p_share_mem[9].addr;
		mdbc_parm.OutInfo.uiOutAddr0 = p_share_mem[6].addr;
		mdbc_parm.OutInfo.uiOutAddr1 = p_share_mem[3].addr;
		mdbc_parm.OutInfo.uiOutAddr2 = p_share_mem[4].addr;
		mdbc_parm.OutInfo.uiOutAddr3 = p_share_mem[5].addr;
	}
#endif
	mdbc_parm.InInfo.uiInAddr0 = p_share_mem[0].addr;
	mdbc_parm.InInfo.uiInAddr1 = p_share_mem[1].addr;
	mdbc_parm.InInfo.uiInAddr2 = p_share_mem[2].addr;
	mdbc_parm.InInfo.uiInAddr3 = p_share_mem[3].addr;
	mdbc_parm.InInfo.uiInAddr4 = p_share_mem[4].addr;
	mdbc_parm.InInfo.uiInAddr5 = p_share_mem[5].addr;
	mdbc_parm.OutInfo.uiOutAddr0 = p_share_mem[6].addr;
	mdbc_parm.OutInfo.uiOutAddr1 = p_share_mem[3].addr;
	mdbc_parm.OutInfo.uiOutAddr2 = p_share_mem[4].addr;
	mdbc_parm.OutInfo.uiOutAddr3 = p_share_mem[5].addr;
	
	mdbc_parm.uiLLAddr          = 0x0;
	mdbc_parm.InInfo.uiLofs0    = MDBC_ALIGN(IMG_WIDTH,4);//160;
	mdbc_parm.InInfo.uiLofs1    = MDBC_ALIGN(IMG_WIDTH,4);//160;
	mdbc_parm.Size.uiMdbcWidth  = IMG_WIDTH;
	mdbc_parm.Size.uiMdbcHeight = IMG_HEIGHT;

	switch (sensi) {
	case LOW_SENSI:
		md_set_para_low_sensitivity(&mdbc_parm);
		break;
	case MED_SENSI:
		md_set_para_medium_sensitivity(&mdbc_parm);
		break;
	case HIGH_SENSI:
		md_set_para_high_sensitivity(&mdbc_parm);
		break;
	case DEFAULT_SENSI:
	default:
		md_set_para_default_sensitivity(&mdbc_parm);
		break;
	}

	ret = vendor_md_set(VENDOR_MD_PARAM_ALL, &mdbc_parm);
	if (HD_OK != ret) {
		printf("set img fail, error code = %d\r\n", ret);
	}
	return ret;
}


MAIN(argc, argv)
{
    HD_RESULT ret;
	MEM_RANGE share_mem[SHARE_MEM_NUM] = {0};
	int frmidx = 0,idx;
	UINT32 pattern_end_id = 100;
	UINT32 is_Init=0;
	VENDOR_MD_TRIGGER_PARAM md_trig_param;
	UINT32 file_size = 0;
	char in_file1[64];
	char in_file2[64];
#if DEBUG_FILE
	int base_id = 3051;
	char out_file1[64];
	char out_file2[64];
	char out_file3[64];
	char out_file4[64];
	FILE  *fd;
#endif
	/*
	char in_file3[64];
	char in_file4[64];
	char in_file5[64];
	char in_file6[64];
	*/
#if OUTPUT_BMP
	char ImgFilePath[64];
#endif

	if(argc > 3){
		idx = 1;
		if (argc > idx) {
			sscanf(argv[idx++], "%lu", &IMG_WIDTH);
		}
		if (argc > idx) {
			sscanf(argv[idx++], "%lu", &IMG_HEIGHT);
		}
		if (argc > idx) {
			sscanf(argv[idx++], "%lu", &pattern_end_id);
		}
	}else{
		IMG_WIDTH = 160;
		IMG_HEIGHT = 120;
		pattern_end_id = 10;
	}
	printf("IMG_WIDTH = %lu,IMG_HEIGHT = %lu,pattern_end_id = %lu\r\n", IMG_WIDTH,IMG_HEIGHT,pattern_end_id);
	IMG_BUF_SIZE = (IMG_WIDTH * IMG_HEIGHT);
	printf("IMG_BUF_SIZE = %lu\r\n", IMG_BUF_SIZE);

	/* init common module */
	ret = hd_common_init(0);
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto comm_init_fail;
    }
	/* init memory */
	ret = mem_init();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto mem_init_fail;
    }
	/* init share memory */
	ret = share_memory_init(share_mem);
	if (ret != HD_OK) {
		printf("mem_init fail=%d\n", ret);
		goto exit;
	}

	/* init gfx for scale*/
	ret = hd_gfx_init();
    if (HD_OK != ret) {
        printf("hd_gfx_init fail\r\n");
        goto exit;
    }

	ret = vendor_md_init();
	if (HD_OK != ret) {
		printf("init fail, error code = %d\r\n", ret);
		goto exit;
	}
/*
	{
	ininfo.uiInAddr0 = share_mem[0].addr;
	ininfo.uiInAddr1 = share_mem[1].addr;
	ininfo.uiInAddr2 = share_mem[2].addr;
	ininfo.uiInAddr3 = share_mem[3].addr;
	ininfo.uiInAddr4 = share_mem[4].addr;
	ininfo.uiInAddr5 = share_mem[5].addr;
	ininfo.uiLofs0   = 60;//160;
	ininfo.uiLofs1   = 60;//160;
	outinfo.uiOutAddr0 = share_mem[6].addr;
	outinfo.uiOutAddr1 = share_mem[7].addr;
	outinfo.uiOutAddr2 = share_mem[8].addr;
	outinfo.uiOutAddr3 = share_mem[9].addr;
	mdbc_parm.mode = 0;
	mdbc_parm.controlEn.update_nei_en = 0;//1;//p_mdbc_reg->MDBC_Register_0004.bit.BC_UPDATE_NEI_EN;
	mdbc_parm.controlEn.deghost_en    = 1;//p_mdbc_reg->MDBC_Register_0004.bit.BC_DEGHOST_EN;
	mdbc_parm.controlEn.roi_en0       = 0;//p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN0;
	mdbc_parm.controlEn.roi_en1       = 0;//p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN1;
	mdbc_parm.controlEn.roi_en2       = 0;//p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN2;
	mdbc_parm.controlEn.roi_en3       = 0;//p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN3;
	mdbc_parm.controlEn.roi_en4       = 0;//p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN4;
	mdbc_parm.controlEn.roi_en5       = 0;//p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN5;
	mdbc_parm.controlEn.roi_en6       = 0;//p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN6;
	mdbc_parm.controlEn.roi_en7       = 0;//p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN7;
	mdbc_parm.controlEn.chksum_en     = 0;//p_mdbc_reg->MDBC_Register_0004.bit.CHKSUM_EN;
	mdbc_parm.controlEn.bgmw_save_bw_en = 0;//p_mdbc_reg->MDBC_Register_0004.bit.BGMW_SAVE_BW_EN;

	mdbc_parm.InInfo.uiInAddr0 = ininfo.uiInAddr0;
	mdbc_parm.InInfo.uiInAddr1 = ininfo.uiInAddr1;
	mdbc_parm.InInfo.uiInAddr2 = ininfo.uiInAddr2;
	mdbc_parm.InInfo.uiInAddr3 = ininfo.uiInAddr3;
	mdbc_parm.InInfo.uiInAddr4 = ininfo.uiInAddr4;
	mdbc_parm.InInfo.uiInAddr5 = ininfo.uiInAddr5;
	mdbc_parm.InInfo.uiLofs0   = ininfo.uiLofs0;
	mdbc_parm.InInfo.uiLofs1   = ininfo.uiLofs1;
	mdbc_parm.OutInfo.uiOutAddr0 = outinfo.uiOutAddr0;
	mdbc_parm.OutInfo.uiOutAddr1 = outinfo.uiOutAddr1;
	mdbc_parm.OutInfo.uiOutAddr2 = outinfo.uiOutAddr2;
	mdbc_parm.OutInfo.uiOutAddr3 = outinfo.uiOutAddr3;

	mdbc_parm.Size.uiMdbcWidth  = width;
	mdbc_parm.Size.uiMdbcHeight = height;

	mdbc_parm.MdmatchPara.lbsp_th    = 0xdf;//0x0;//p_mdbc_reg->MDBC_Register_004c.bit.LBSP_TH;//0x48;
	mdbc_parm.MdmatchPara.d_colour   = 0x4;//0x6;//p_mdbc_reg->MDBC_Register_004c.bit.D_COLOUR;//0xb;
	mdbc_parm.MdmatchPara.r_colour   = 0xc;//0x1e;//p_mdbc_reg->MDBC_Register_004c.bit.R_COLOUR;//0xf;
	mdbc_parm.MdmatchPara.d_lbsp     = 0xa;//0x3;//p_mdbc_reg->MDBC_Register_004c.bit.D_LBSP;//0xf;
	mdbc_parm.MdmatchPara.r_lbsp     = 0x3;//0x5;//p_mdbc_reg->MDBC_Register_004c.bit.R_LBSP;//0x5;
	mdbc_parm.MdmatchPara.model_num  = 0x8;//bg_model_num;
	mdbc_parm.MdmatchPara.t_alpha    = 0xfb;//0x33;//p_mdbc_reg->MDBC_Register_0050.bit.T_ALPHA;//0x82;
	mdbc_parm.MdmatchPara.dw_shift   = 0x0;//0x4;//p_mdbc_reg->MDBC_Register_0050.bit.DW_SHIFT;//0;
	mdbc_parm.MdmatchPara.dlast_alpha= 0x1c5;//0x28;//p_mdbc_reg->MDBC_Register_0050.bit.D_LAST_ALPHA;//0x1a7;
	mdbc_parm.MdmatchPara.min_match  = 0x2;//p_mdbc_reg->MDBC_Register_0054.bit.BC_MIN_MATCH;//4;
	mdbc_parm.MdmatchPara.dlt_alpha  = 0x194;//0xa;//p_mdbc_reg->MDBC_Register_0054.bit.DLT_ALPHA;//0x17a;
	mdbc_parm.MdmatchPara.dst_alpha  = 0x37a;//0x28;//p_mdbc_reg->MDBC_Register_0054.bit.DST_ALPHA;//0x23d;
	mdbc_parm.MdmatchPara.uv_thres   = 0x4;//0x6;//p_mdbc_reg->MDBC_Register_0058.bit.BC_UV_THRES;//2;
	mdbc_parm.MdmatchPara.s_alpha    = 0x195;//0x28;//p_mdbc_reg->MDBC_Register_0058.bit.S_ALPHA;//0xd8;
	mdbc_parm.MdmatchPara.dbg_lumDiff= 0x3197;//0x0;//p_mdbc_reg->MDBC_Register_005c.bit.DBG_LUM_DIFF;//0x22b4;
	mdbc_parm.MdmatchPara.dbg_lumDiff_en = 0x1;//0x0;//p_mdbc_reg->MDBC_Register_005c.bit.DBG_LUM_DIFF_EN;//1;

	mdbc_parm.MorPara.th_ero     = 0x0;//0x8;//p_mdbc_reg->MDBC_Register_0064.bit.TH_ERO;//6;
	mdbc_parm.MorPara.th_dil     = 0x6;//0x0;//p_mdbc_reg->MDBC_Register_0064.bit.TH_DIL;//6;
	mdbc_parm.MorPara.mor_sel0   = 0x2;//0x0;//p_mdbc_reg->MDBC_Register_0064.bit.MOR_SEL0;//KDRV_Erode;//2;
	mdbc_parm.MorPara.mor_sel1   = 0x0;//0x1;//p_mdbc_reg->MDBC_Register_0064.bit.MOR_SEL1;//KDRV_Median;//0;
	mdbc_parm.MorPara.mor_sel2   = 0x0;//0x2;//p_mdbc_reg->MDBC_Register_0064.bit.MOR_SEL2;//KDRV_Erode;//2;
	mdbc_parm.MorPara.mor_sel3   = 0x1;//0x3;//p_mdbc_reg->MDBC_Register_0064.bit.MOR_SEL3;//KDRV_Median;//0;

	mdbc_parm.UpdPara.minT           = 0x5c;//0x2;//p_mdbc_reg->MDBC_Register_0068.bit.BC_MIN_T;//0x3e;
	mdbc_parm.UpdPara.maxT           = 0xe1;//0xff;//p_mdbc_reg->MDBC_Register_0068.bit.BC_MAX_T;//0x4a;
	mdbc_parm.UpdPara.maxFgFrm       = 0xae;//0xff;//p_mdbc_reg->MDBC_Register_0068.bit.BC_MAX_FG_FRM;//0x6c;
	mdbc_parm.UpdPara.deghost_dth    = 0x16;//0x5;//p_mdbc_reg->MDBC_Register_006c.bit.BC_DEGHOST_DTH;//0x1d;
	mdbc_parm.UpdPara.deghost_sth    = 0x37;//0xfa;//p_mdbc_reg->MDBC_Register_006c.bit.BC_DEGHOST_STH;//0x65;
	mdbc_parm.UpdPara.stable_frm     = 0xaa;//0x78;//p_mdbc_reg->MDBC_Register_0070.bit.BC_STABLE_FRAME;//0x3b;
	mdbc_parm.UpdPara.update_dyn     = 0xb2;//0x80;//p_mdbc_reg->MDBC_Register_0070.bit.BC_UPDATE_DYN;//0x82;
	mdbc_parm.UpdPara.va_distth      = 32;
	mdbc_parm.UpdPara.t_distth       = 24;
	mdbc_parm.UpdPara.dbg_frmID      = 0x2;//0x0;//p_mdbc_reg->MDBC_Register_0074.bit.DBG_FRM_ID;//2;
	mdbc_parm.UpdPara.dbg_frmID_en   = 0x1;//0x0;//p_mdbc_reg->MDBC_Register_0074.bit.DBG_FRM_ID_EN;//1;
	mdbc_parm.UpdPara.dbg_rnd        = 0x1070;//0x0;//p_mdbc_reg->MDBC_Register_0078.bit.DBG_RND;//0x243;
	mdbc_parm.UpdPara.dbg_rnd_en     = 0x1;//0x0;//p_mdbc_reg->MDBC_Register_0078.bit.DBG_RND_EN;//1;
	}
*/
	
	md_trig_param.is_nonblock = 0;
	md_trig_param.time_out_ms = 0;
	for(frmidx = 0; frmidx <= (int)pattern_end_id; frmidx++)
	{
		printf("------------ frmidx = %d ------------\r\n", frmidx);

		/// load file
#if defined(__FREERTOS)
		snprintf(in_file1, 64, "A:\\MDBCP\\DI\\pic_%04d\\dram_in_y.bin", frmidx);
#else
		snprintf(in_file1, 64, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_y.bin", frmidx);
#endif
		file_size = md_load_file(in_file1, share_mem[0].va);
		if (file_size == 0) {
			printf("[ERR]load dram_in_y.bin : %s\r\n", in_file1);
		}

#if defined(__FREERTOS)
		snprintf(in_file2, 64, "A:\\MDBCP\\DI\\pic_%04d\\dram_in_uv.bin", frmidx);
#else
		snprintf(in_file2, 64, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_uv.bin", frmidx);
#endif
		file_size = md_load_file(in_file2, share_mem[1].va);
		if (file_size == 0) {
			printf("[ERR]load dram_in_uv.bin : %s\r\n", in_file2);
		}
/*
		if(mdbc_parm.mode == 1){
			snprintf(in_file3, 64, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_uv_pre.bin", frmidx);
			file_size = md_load_file(in_file3, share_mem[2].va);
			if (file_size == 0) {
				printf("[ERR]load dram_in_uv_pre.bin : %s\r\n", in_file3);
			}
			snprintf(in_file4, 64, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_bgYUV.bin", frmidx);
			file_size = md_load_file(in_file4, share_mem[3].va);
			if (file_size == 0) {
				printf("[ERR]load dram_in_bgYUV.bin : %s\r\n", in_file4);
			}
			snprintf(in_file5, 64, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_var1.bin", frmidx);
			file_size = md_load_file(in_file5, share_mem[4].va);
			if (file_size == 0) {
				printf("[ERR]load dram_in_var1.bin : %s\r\n", in_file5);
			}
			snprintf(in_file6, 64, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_var2.bin", frmidx);
			file_size = md_load_file(in_file6, share_mem[5].va);
			if (file_size == 0) {
				printf("[ERR]load dram_in_var2.bin : %s\r\n", in_file6);
			}
		}
*/

		/// set to engine
		md_set_para(share_mem, is_Init,g_md_sensi);
		/// engine fire
		ret = vendor_md_trigger(&md_trig_param);
		if (HD_OK != ret) {
			printf("trigger fail, error code = %d\r\n", ret);
			break;
		}
		memcpy((UINT32 *)share_mem[2].va , (UINT32 *)share_mem[1].va , IMG_BUF_SIZE/2);

		if(is_Init == 1) {
			hd_common_mem_flush_cache((VOID *)share_mem[6].va, ((IMG_BUF_SIZE+7)/8));
			bc_reorgS1((UINT8*)share_mem[6].va,(UINT8*)share_mem[7].va, IMG_WIDTH, IMG_HEIGHT);
#if OUTPUT_BMP
#if defined(__FREERTOS)
		snprintf(ImgFilePath, 64, "A:\\MDBCP\\Debug\\output_bmp\\output_%04d.bmp", frmidx);
#else
		snprintf(ImgFilePath, 64, "//mnt//sd//MDBCP//Debug//output_bmp//output_%04d.bmp", frmidx);
#endif
			bc_writebmpfile(ImgFilePath, (UINT8*)share_mem[7].va, IMG_WIDTH, IMG_HEIGHT, 8);
#endif

#if DEBUG_FILE
#if defined(__FREERTOS)
		snprintf(out_file1, 64, "A:\\MDBCP\\DO\\pic_%04d\\dram_out_s1.bin", base_id+frmidx);
#else
		snprintf(out_file1, 64, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_s1.bin", base_id+frmidx);
#endif
			fd = fopen(out_file1, "wb");
			if (!fd) {
				printf("cannot read %s\r\n", out_file1);
			} else {
    			file_size = fwrite((const void *)share_mem[6].va,1,((IMG_BUF_SIZE+7)/8),fd);
    			if (file_size == 0) {
    				printf("load dram_out_s1.bin : %s\r\n", out_file1);
    			}
    			fclose(fd);
			}

			hd_common_mem_flush_cache((VOID *)share_mem[3].va, IMG_BUF_SIZE*40);
#if defined(__FREERTOS)
		snprintf(out_file2, 64, "A:\\MDBCP\\DO\\pic_%04d\\dram_out_bgYUV.bin", base_id+frmidx);
#else
		snprintf(out_file2, 64, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_bgYUV.bin", base_id+frmidx);
#endif
			fd = fopen(out_file2, "wb");
			if (!fd) {
				printf("cannot read %s\r\n", out_file2);
			} else {
				file_size = fwrite((const void *)share_mem[3].va,1,IMG_BUF_SIZE*40,fd);
    			if (file_size == 0) {
    				printf("load dram_out_bgYUV.bin : %s\r\n", out_file2);
    			}
    			fclose(fd);
			}
#endif
		}
#if DEBUG_FILE
		hd_common_mem_flush_cache((VOID *)share_mem[4].va, IMG_BUF_SIZE*6);
#if defined(__FREERTOS)
		snprintf(out_file3, 64, "A:\\MDBCP\\DO\\pic_%04d\\dram_out_var1.bin", base_id+frmidx);
#else
		snprintf(out_file3, 64, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_var1.bin", base_id+frmidx);
#endif
		fd = fopen(out_file3, "wb");
		if (!fd) {
			printf("cannot read %s\r\n", out_file3);
		} else {
    		file_size = fwrite((const void *)share_mem[4].va,1,IMG_BUF_SIZE*6,fd);
    		if (file_size == 0) {
    			printf("load dram_out_var1.bin : %s\r\n", out_file3);
    		}
    		fclose(fd);
		}

		hd_common_mem_flush_cache((VOID *)share_mem[5].va, ((IMG_BUF_SIZE+15)/16)*16*12);
#if defined(__FREERTOS)
		snprintf(out_file4, 64, "A:\\MDBCP\\DO\\pic_%04d\\dram_out_var2.bin", base_id+frmidx);
#else
		snprintf(out_file4, 64, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_var2.bin", base_id+frmidx);
#endif
		fd = fopen(out_file4, "wb");
		if (!fd) {
			printf("cannot read %s\r\n", out_file4);
		} else {
    		file_size = fwrite((const void *)share_mem[5].va,1,((IMG_BUF_SIZE+15)/16)*16*12,fd);
    		if (file_size == 0) {
    			printf("load dram_out_var2.bin : %s\r\n", out_file4);
    		}
    		fclose(fd);
		}
#endif
		//ping_pong_id = (ping_pong_id+1)%2;
		if(is_Init==0)is_Init=1;
	}

	ret = vendor_md_uninit();
	if (HD_OK != ret)
		printf("uninit fail, error code = %d\r\n", ret);


exit:
	ret = hd_gfx_uninit();
	if (HD_OK != ret) {
		printf("hd_gfx_uninit fail\r\n");
	}

	ret = share_memory_exit(share_mem);
	if (ret != HD_OK) {
		printf("mem_uninit fail=%d\n", ret);
	}

	ret = mem_exit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

mem_init_fail:
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        printf("uninit fail=%d\n", ret);
    }
comm_init_fail:
	return 0;
}