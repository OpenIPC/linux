#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "tse_api.h"
#include "tse_drv.h"
#include "tse_dbg.h"
#include "tse_main.h"
#include "tse_platform_int.h"
#include "comm/nvtmem.h"

#if 0
#endif
#if (NVT_TSE_TEST_CMD == 1)
int nvt_tse_set_dbglevel(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int value;

	if (argc == 0) {
		DBG_ERR("argc must > 0\n");
		return -1;
	}

	if (kstrtouint(pargv[0], 10, &value)) {
		DBG_ERR("convert fail\n");
		return -1;
	}

	if ((value > NVT_DBG_USER) || (value < NVT_DBG_FATAL)) {
		DBG_ERR("value(%d) overflow\n", value);
		return -1;
	}
	DBG_IND("value = %d\n", value);
	tse_debug_level = value;
	return 0;
}

int nvt_tse_get_dbglevel(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	DBG_DUMP("tse_debug_level = %d\n", tse_debug_level);
	return 0;
}

#if 0
#endif

int nvt_tse_reset(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	DBG_DUMP("reset enter\n");
	clk_unprepare(pmodule_info->pclk[0]);
	clk_prepare(pmodule_info->pclk[0]);
	DBG_DUMP("reset exit\n");
	return 0;
}

#if 0
#endif
static INT32 nvt_tse_set_const_verify(UINT32 addr, UINT32 data_size, UINT32 data_val)
{
	UINT32 i;
	UINT32 *ptr32;

	/* compare data */
	ptr32 = (UINT32 *)addr;
	for (i = 0; i < (data_size >> 2); i ++) {
		if (ptr32[i] != data_val) {
			DBG_DUMP("data mismatch addr:0x%.8x (0x%.8x, 0x%.8x)\n", (UINT32)&ptr32[i], ptr32[i], data_val);
			return -1;
		}
	}
	DBG_DUMP("Compare PASS\n");
	return 0;
}

int nvt_tse_mem_set_test(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int ret;
 	size_t out_size;
	struct nvt_fmem_mem_info_t outbuf = {0};
	void *out_handle;
	UINT32 ctex_ptn;

	//init
	out_size = (2 * 1024 * 1024); //2MB
	out_handle = NULL;
	ret = 0;
	ctex_ptn = 0xFFFEFDFC;

	DBG_DUMP("memset test enter\n");

	//allocate buffer
	ret = nvt_fmem_mem_info_init(&outbuf, NVT_FMEM_ALLOC_CACHE, out_size, NULL);
	if (ret != 0) {
		DBG_ERR("nvt_fmem_mem_info_init fail\n");
		ret = -1;
		goto err;
	}

	out_handle = nvtmem_alloc_buffer(&outbuf);
	if ((out_handle == NULL) || (outbuf.vaddr == NULL)) {
		DBG_ERR("outbuf(0x%.8x) alloc fail\n", outbuf.size);
		ret = -1;
		goto err;
	}
	DBG_DUMP("outbuf(0x%.8x, 0x%.8x, 0x%.8x)\n", (UINT32)outbuf.vaddr, outbuf.paddr, outbuf.size);

	//reset buffer
	memset(outbuf.vaddr, 0x5A,  outbuf.size);
	fmem_dcache_sync(outbuf.vaddr,  outbuf.size, DMA_TO_DEVICE);

	tse_open();
	tse_setConfig(HWCOPY_CFG_ID_CMD, HWCOPY_LINEAR_SET);
	tse_setConfig(HWCOPY_CFG_ID_CTEX, ctex_ptn);
	tse_setConfig(HWCOPY_CFG_ID_DST_ADDR, (UINT32)outbuf.vaddr);
	tse_setConfig(HWCOPY_CFG_ID_SRC_LEN, outbuf.size);
	tse_start(TRUE, TSE_MODE_HWCOPY);
	if (outbuf.size != tse_getConfig(HWCOPY_CFG_ID_TOTAL_LEN)) {
		DBG_ERR("size NG (0x%.8x, 0x%.8x)\n", outbuf.size, tse_getConfig(HWCOPY_CFG_ID_TOTAL_LEN));
	}
	tse_close();

	fmem_dcache_sync(outbuf.vaddr,  outbuf.size, DMA_FROM_DEVICE);
	ret = nvt_tse_set_const_verify((UINT32)outbuf.vaddr, outbuf.size, ctex_ptn);

err:
	if (out_handle) {
		nvtmem_release_buffer(out_handle);
	}

	DBG_DUMP("memset test exit...done(%d)\n", ret);
	return ret;
}

#if 0
#endif
static INT32 nvt_tse_copy_verify(UINT32 src_addr, UINT32 dst_addr, UINT32 size)
{
	UINT32 i;
	UINT8 *ptr_src;
	UINT8 *ptr_dst;

	ptr_src = (UINT8 *)src_addr;
	ptr_dst = (UINT8 *)dst_addr;

	/* compare copy data */
	for (i = 0; i < size; i += 1) {
		if (ptr_src[i] != ptr_dst[i]) {
			DBG_ERR("data mismatch (0x%.8x, 0x%.8x) (0x%.8x, 0x%.8x)\n", ptr_src[i], ptr_dst[i], (UINT32)&ptr_dst[i], (UINT32)&ptr_dst[i]);
			return -1;
		}
	}
	DBG_DUMP("Compare PASS\n");
	return 0;
}

int nvt_tse_mem_cpy_test(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int ret;
 	size_t out_size, in_size;
	struct nvt_fmem_mem_info_t outbuf = {0}, inbuf = {0};
	void *out_handle, *in_handle;

	//init
	in_size = (2 * 1024 * 1024); //2MB
	out_size = (2 * 1024 * 1024); //2MB

	in_handle = NULL;
	out_handle = NULL;
	ret = 0;

	DBG_DUMP("memcpy test enter\n");

	//allocate buffer
	ret = nvt_fmem_mem_info_init(&inbuf, NVT_FMEM_ALLOC_CACHE, in_size, NULL);
	if (ret != 0) {
		DBG_ERR("nvt_fmem_mem_info_init fail\n");
		ret = -1;
		goto err;
	}

	in_handle = nvtmem_alloc_buffer(&inbuf);
	if ((in_handle == NULL) || (inbuf.vaddr == NULL)) {
		DBG_ERR("inbuf(0x%.8x) alloc fail\n", inbuf.size);
		ret = -1;
		goto err;
	}
	DBG_DUMP("inbuf(0x%.8x, 0x%.8x, 0x%.8x)\n", (UINT32)inbuf.vaddr, inbuf.paddr, inbuf.size);

	ret = nvt_fmem_mem_info_init(&outbuf, NVT_FMEM_ALLOC_CACHE, out_size, NULL);
	if (ret != 0) {
		DBG_ERR("nvt_fmem_mem_info_init fail\n");
		ret = -1;
		goto err;
	}

	out_handle = nvtmem_alloc_buffer(&outbuf);
	if ((out_handle == NULL) || (outbuf.vaddr == NULL)) {
		DBG_ERR("outbuf(0x%.8x) alloc fail\n", outbuf.size);
		ret = -1;
		goto err;
	}
	DBG_DUMP("outbuf(0x%.8x, 0x%.8x, 0x%.8x)\n", (UINT32)outbuf.vaddr, outbuf.paddr, outbuf.size);

	if (outbuf.size < inbuf.size) {
		DBG_ERR("out size(0x%.8x) < in size(0x%.8x)\n", outbuf.size, inbuf.size);
		goto err;
	}

	//reset buffer
	memset(inbuf.vaddr, 0x5A,  inbuf.size);
	fmem_dcache_sync(inbuf.vaddr,  inbuf.size, DMA_TO_DEVICE);

	memset(outbuf.vaddr, 0x5A,  outbuf.size);
	fmem_dcache_sync(outbuf.vaddr,  outbuf.size, DMA_TO_DEVICE);

	tse_open();
	tse_setConfig(HWCOPY_CFG_ID_CMD, HWCOPY_LINEAR_COPY);
	tse_setConfig(HWCOPY_CFG_ID_SRC_ADDR, (UINT32)inbuf.vaddr);
	tse_setConfig(HWCOPY_CFG_ID_DST_ADDR, (UINT32)outbuf.vaddr);
	tse_setConfig(HWCOPY_CFG_ID_SRC_LEN, inbuf.size);
	tse_start(TRUE, TSE_MODE_HWCOPY);
	if (outbuf.size != tse_getConfig(HWCOPY_CFG_ID_TOTAL_LEN)) {
		DBG_ERR("size NG (0x%.8x, 0x%.8x)\n", inbuf.size, tse_getConfig(HWCOPY_CFG_ID_TOTAL_LEN));
	}
	tse_close();

	fmem_dcache_sync(outbuf.vaddr,  outbuf.size, DMA_FROM_DEVICE);
	ret = nvt_tse_copy_verify((UINT32)inbuf.vaddr, (UINT32)outbuf.vaddr, inbuf.size);
err:
	if (in_handle) {
		nvtmem_release_buffer(in_handle);
	}

	if (out_handle) {
		nvtmem_release_buffer(out_handle);
	}

	DBG_DUMP("memcpy test exit...done(%d)\n", ret);
	return ret;
}

#if 0
#endif
static UINT32 nvt_tse_mux_cal_out_size(UINT32 in_size, UINT32 payload, UINT32 ts1_183_en)
{
	UINT32 rt;
	rt = ((in_size + (payload - 1)) / payload) * 188;
	if (ts1_183_en == 1) {
		//do nothing
	} else {
		if ((payload == 184) && ((in_size % 184) == 183)) {
			rt += 188;
		}
	}
	return rt;
}

typedef struct {
	UINT32 con_cnt : 4;
	UINT32 adap_ctl : 2;
	UINT32 scrambling : 2;
	UINT32 pid : 13;
	UINT32 tp : 1;
	UINT32 start_ind : 1;
	UINT32 tei : 1;
	UINT32 sync : 8;
} TS_H_INFO;

static INT32 nvt_tse_mux_verify(UINT32 DstAddr, UINT32 SrcAddr, UINT32 SrcSize, TS_H_INFO header, UINT32 PayloadSize, UINT32 ts1_183_en)
{
	UINT8   *pDst8, *pSrc8;
	UINT32  seg, SrcLoc;
	UINT32  i, j, temp;

	pDst8   = (UINT8 *) DstAddr;
	pSrc8   = (UINT8 *) SrcAddr;

	seg = (SrcSize + (PayloadSize - 1)) / PayloadSize;
	if ((PayloadSize == 184) && ((SrcSize % 184) == 183)) {
		if (ts1_183_en == 1) {
		} else {
			seg++;
		}
	}

	// Compare SYNC
	temp = tse_getConfig(TSMUX_CFG_ID_SYNC_BYTE);
	for (i = 0; i < seg; i++) {
		if (pDst8[i * 188] != temp) {
			DBG_ERR("SYNC Code Error! Addr=0x%.8x SEG=%d 0x%.8x!=0x%.8x\n", (UINT32)&pDst8[i * 188], i, pDst8[i * 188], temp);
			return -1;
		}
	}

	// Compare PID Higher 5 bits
	temp = tse_getConfig(TSMUX_CFG_ID_PID);
	temp = temp >> 8;
	for (i = 0; i < seg; i++) {
		if ((pDst8[(i * 188) + 1] & 0x1F) != temp) {
			DBG_ERR("^RPID Code Error! Addr=0x%.8x SEG=%d\n", (UINT32)&pDst8[(i * 188) + 1], i);
			return -1;
		}
	}
	// Compare PID Lower Byte
	temp = tse_getConfig(TSMUX_CFG_ID_PID);
	temp = temp & 0xFF;
	for (i = 0; i < seg; i++) {
		if (pDst8[(i * 188) + 2] != temp) {
			DBG_ERR("^RPID Code Error! Addr=0x%.8x SEG=%d\n", (UINT32)&pDst8[(i * 188) + 2], i);
			return -1;
		}
	}

	// Compare TEI
	temp = tse_getConfig(TSMUX_CFG_ID_TEI);
	for (i = 0; i < seg; i++) {
		if ((pDst8[(i * 188) + 1] & 0x80) != (temp << 7)) {
			DBG_ERR("^RTEI Error! Addr=0x%.8x SEG=%d\n", (UINT32)&pDst8[(i * 188) + 1], i);
			return -1;
		}
	}

	// Compare TP
	temp = tse_getConfig(TSMUX_CFG_ID_TP);
	for (i = 0; i < seg; i++) {
		if ((pDst8[(i * 188) + 1] & 0x20) != (temp << 5)) {
			DBG_ERR("^RTP Error! Addr=0x%.8x SEG=%d\n", (UINT32)&pDst8[(i * 188) + 1], i);
			return -1;
		}
	}

	// Compare Scramble Control
	temp = tse_getConfig(TSMUX_CFG_ID_SCRAMBLECTRL);
	for (i = 0; i < seg; i++) {
		if ((pDst8[(i * 188) + 3] & 0xC0) != (temp << 6)) {
			DBG_ERR("^RSCRAMBLECTRL Error! Addr=0x%.8x SEG=%d\n", (UINT32)&pDst8[(i * 188) + 3], i);
			return -1;
		}
	}

	// Compare CONT_COUNTER
	for (i = 0; i < seg; i++) {
		temp = header.con_cnt + i;
		if ((pDst8[(i * 188) + 3] & 0xF) != (temp & 0xF)) {
			DBG_ERR("^RCONT_COUNTER Error! Addr=0x%.8x SEG=%d\n", (UINT32)&pDst8[(i * 188) + 3], i);
			return -1;
		}
	}

	if (((temp + 1) & 0xf) != tse_getConfig(TSMUX_CFG_ID_CON_CURR_CNT)) {
		DBG_ERR("^Rlast CONT_COUNTER Error! last=0x%.8x reg=0x%.8x\n", temp, tse_getConfig(TSMUX_CFG_ID_CON_CURR_CNT));
		return -1;
	}

	// Compare START_INDICATOR
	for (i = 0; i < seg; i++) {
		if (i == 0) {
			if ((pDst8[(i * 188) + 1] & 0x40) != (header.start_ind << 6)) {
				DBG_ERR("^RSTART_INDICATOR Error! Addr=0x%.8x SEG=%d\n", (UINT32)&pDst8[(i * 188) + 1], i);
				return -1;
			}
		} else {
			if ((pDst8[(i * 188) + 1] & 0x40) != 0) {
				DBG_ERR("^RSTART_INDICATOR Error! Addr=0x%.8x SEG=%d\n", (UINT32)&pDst8[(i * 188) + 1], i);
				return -1;
			}
		}
	}

	SrcLoc = 0;

	// COMPARE DATA and Adapt field
	for (i = 0; i < seg; i++) {
		//emu_msg(("."));

		if ((pDst8[(i * 188) + 3] & 0x30) == 0x10) {
			// No Adapt Field -> compare 184 Bytes Data

			for (j = 0; j < PayloadSize; j++) {
				if (pDst8[(i * 188) + 4 + j] != pSrc8[SrcLoc + j]) {
					DBG_ERR("^RDATA Error! DstAddr=0x%.8x SrcAddr=0x%.8x  --", (UINT32)&pDst8[(i * 188) + 4 + j], (UINT32)&pSrc8[SrcLoc + j]);
					DBG_ERR("^RExpect 0x%.2x    But Got 0x%.2x\n", pSrc8[SrcLoc + j], pDst8[(i * 188) + 4 + j]);
					return -1;
				}
			}
			SrcLoc += PayloadSize;
		} else if ((pDst8[(i * 188) + 3] & 0x30) == 0x30) {

			UINT32 segSize, StuffSize;

			// 1/2Bytes Adapt Field
			StuffSize = pDst8[(i * 188) + 4];
			if (StuffSize == 0) {
				segSize = 183;
			} else {
				segSize = 188 - 4 - 1 - StuffSize;
			}

			if (segSize != 183) {
				temp = tse_getConfig(TSMUX_CFG_ID_ADAPT_FLAGS);
				if (temp != pDst8[(i * 188) + 5]) {
					DBG_ERR("^RAPAPT FLAGS Error! Addr=0x%08X SEG=%d\n", (UINT32)&pDst8[(i * 188) + 5], i);
					return -1;
				}
			}

			if (StuffSize) {
				// Compare stuff
				temp = tse_getConfig(TSMUX_CFG_ID_STUFF_VAL);
				for (j = 0; j < (UINT32)(StuffSize - 1); j++) {
					if (pDst8[(i * 188) + 6 + j] != temp) {
						DBG_ERR("^RSTUFF Error! DstAddr=0x%.8x\n", (UINT32)&pDst8[(i * 188) + 6 + j]);
						return -1;
					}
				}
				// Compare DATA
				for (j = 0; j < segSize; j++) {
					if (pDst8[(i * 188) + 5 + StuffSize + j] != pSrc8[SrcLoc + j]) {
						DBG_ERR("^RDATA Error! DstAddr=0x%08X SrcAddr=0x%08X\n", (UINT32)&pDst8[(i * 188) + 5 + StuffSize + j], (UINT32)&pSrc8[SrcLoc + j]);
						return -1;
					}
				}
			} else {
				// Compare DATA
				for (j = 0; j < segSize; j++) {
					if (pDst8[(i * 188) + 5 + j] != pSrc8[SrcLoc + j]) {
						DBG_ERR("^RDATA Error! DstAddr=0x%08X SrcAddr=0x%08X\n", (UINT32)&pDst8[(i * 188) + 5 + j], (UINT32)&pSrc8[SrcLoc + j]);
						return -1;
					}
				}
			}

			SrcLoc += segSize;
		} else {
			DBG_ERR("^RNo Support Adaptation Field Value.\n");
			return -1;
		}
	}

	DBG_DUMP("Compare PASS\n");
	return 0;
}

static INT32 nvt_tse_demux_verify(UINT32 addr, UINT32 size, UINT32 start_value)
{
	UINT32 i;
	UINT8 *ptr, data;

	ptr = (UINT8 *)addr;
	data = start_value;
	for (i = 0; i < size; i ++) {
		if (*ptr != data) {
			DBG_ERR("data mismatch 0x%.8x\n", (UINT32)ptr);
			return -1;
		}
		ptr += 1;
		data += 1;
	}
	DBG_DUMP("Compare PASS\n");
	return 0;
}


int nvt_tse_mux_demux_test(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int ret, i, mux_out_size;
 	size_t out_size, in_size;
	struct nvt_fmem_mem_info_t outbuf = {0}, inbuf = {0};
	void *out_handle, *in_handle;

	UINT8 *ptr;
	TS_H_INFO header;
	TSE_BUF_INFO tse_in_buf, tse_out_buf;
	UINT32 payload, stuff_val, adapt_field_flags, test_size;

	//init
	test_size = (184 * 100);
	in_size = (2 * 1024 * 1024); //2MB
	out_size = (2 * 1024 * 1024); //2MB

	in_handle = NULL;
	out_handle = NULL;
	ret = 0;

	DBG_DUMP("mux test enter\n");

	//allocate buffer
	ret = nvt_fmem_mem_info_init(&inbuf, NVT_FMEM_ALLOC_CACHE, in_size, NULL);
	if (ret != 0) {
		DBG_ERR("nvt_fmem_mem_info_init fail\n");
		ret = -1;
		goto err;
	}

	in_handle = nvtmem_alloc_buffer(&inbuf);
	if ((in_handle == NULL) || (inbuf.vaddr == NULL)) {
		DBG_ERR("inbuf(0x%.8x) alloc fail\n", inbuf.size);
		ret = -1;
		goto err;
	}
	DBG_DUMP("inbuf(0x%.8x, 0x%.8x, 0x%.8x)\n", (UINT32)inbuf.vaddr, inbuf.paddr, inbuf.size);

	ret = nvt_fmem_mem_info_init(&outbuf, NVT_FMEM_ALLOC_CACHE, out_size, NULL);
	if (ret != 0) {
		DBG_ERR("nvt_fmem_mem_info_init fail\n");
		ret = -1;
		goto err;
	}

	out_handle = nvtmem_alloc_buffer(&outbuf);
	if ((out_handle == NULL) || (outbuf.vaddr == NULL)) {
		DBG_ERR("outbuf(0x%.8x) alloc fail\n", outbuf.size);
		ret = -1;
		goto err;
	}
	DBG_DUMP("outbuf(0x%.8x, 0x%.8x, 0x%.8x)\n", (UINT32)outbuf.vaddr, outbuf.paddr, outbuf.size);

	if (outbuf.size < inbuf.size) {
		DBG_ERR("out size(0x%.8x) < in size(0x%.8x)\n", outbuf.size, inbuf.size);
		goto err;
	}

	/***************** mux test *****************/
	//reset buffer
	ptr = (UINT8 *)((UINT32)inbuf.vaddr);
	for (i = 0; i < inbuf.size; i ++) {
		ptr[i] = (i & 0xff);
	}
	fmem_dcache_sync(inbuf.vaddr,  inbuf.size, DMA_TO_DEVICE);

	memset(outbuf.vaddr, 0x5A,  outbuf.size);
	fmem_dcache_sync(outbuf.vaddr,  outbuf.size, DMA_TO_DEVICE);

	tse_in_buf.addr = (UINT32)inbuf.vaddr;
	tse_in_buf.size = test_size;
	tse_in_buf.pnext = NULL;

	tse_out_buf.addr = (UINT32)outbuf.vaddr;
	tse_out_buf.size = outbuf.size;
	tse_out_buf.pnext = NULL;

	header.sync = 0x47;
	header.tei = 0;
	header.start_ind = 1;
	header.tp = 0;
	header.pid = 0x1fff;
	header.scrambling = 0;
	header.adap_ctl = 0;
	header.con_cnt = 0;

	stuff_val = 0xff;
	adapt_field_flags = 0x0;
	payload = 184;

	tse_open();

	tse_setConfig(TSMUX_CFG_ID_PAYLOADSIZE, payload);
	tse_setConfig(TSMUX_CFG_ID_SRC_INFO, (UINT32)&tse_in_buf);
	tse_setConfig(TSMUX_CFG_ID_DST_INFO, (UINT32)&tse_out_buf);
	tse_setConfig(TSMUX_CFG_ID_SYNC_BYTE, header.sync);
	tse_setConfig(TSMUX_CFG_ID_CONTINUITY_CNT, header.con_cnt);
	tse_setConfig(TSMUX_CFG_ID_PID, header.pid);
	tse_setConfig(TSMUX_CFG_ID_TEI, header.tei);
	tse_setConfig(TSMUX_CFG_ID_TP, header.tp);
	tse_setConfig(TSMUX_CFG_ID_SCRAMBLECTRL, header.scrambling);
	tse_setConfig(TSMUX_CFG_ID_START_INDICTOR, header.start_ind);
	tse_setConfig(TSMUX_CFG_ID_STUFF_VAL, stuff_val);
	tse_setConfig(TSMUX_CFG_ID_ADAPT_FLAGS, adapt_field_flags);
	tse_setConfig(TSMUX_CFG_ID_LAST_DATA_MUX_MODE, 0);
	tse_start(TRUE, TSE_MODE_TSMUX);

	mux_out_size = nvt_tse_mux_cal_out_size(tse_in_buf.size, 184, 0);
	if (mux_out_size != tse_getConfig(TSMUX_CFG_ID_MUXING_LEN)) {
		DBG_ERR("size NG (0x%.8x, 0x%.8x)\n", mux_out_size, tse_getConfig(TSMUX_CFG_ID_MUXING_LEN));
		ret = -1;
		goto err;
	} else {
		DBG_DUMP("size OK (0x%.8x, 0x%.8x)\n", mux_out_size, tse_getConfig(TSMUX_CFG_ID_MUXING_LEN));
	}

	fmem_dcache_sync(outbuf.vaddr, outbuf.size, DMA_FROM_DEVICE);
	ret = nvt_tse_mux_verify(tse_out_buf.addr, tse_in_buf.addr, tse_in_buf.size, header, payload, 0);

	tse_close();

	if (ret < 0) {
		goto err;
	}

	/***************** demux test *****************/
	//reset buffer
	memset(inbuf.vaddr, 0x5A,  inbuf.size);
	fmem_dcache_sync(inbuf.vaddr,  inbuf.size, DMA_TO_DEVICE);

	tse_in_buf.addr = (UINT32)outbuf.vaddr;
	tse_in_buf.size = mux_out_size;
	tse_in_buf.pnext = NULL;

	tse_out_buf.addr = (UINT32)inbuf.vaddr;
	tse_out_buf.size = inbuf.size;
	tse_out_buf.pnext = NULL;

	tse_open();

	tse_setConfig(TSDEMUX_CFG_ID_SYNC_BYTE, header.sync);
	tse_setConfig(TSDEMUX_CFG_ID_ADAPTATION_FLAG, adapt_field_flags);
	tse_setConfig(TSDEMUX_CFG_ID_IN_INFO, (UINT32)&tse_in_buf);

	tse_setConfig(TSDEMUX_CFG_ID_PID0_ENABLE, ENABLE);
	tse_setConfig(TSDEMUX_CFG_ID_PID0_VALUE, header.pid);
	tse_setConfig(TSDEMUX_CFG_ID_CONTINUITY0_MODE, 0);
	tse_setConfig(TSDEMUX_CFG_ID_CONTINUITY0_VALUE, 0);
	tse_setConfig(TSDEMUX_CFG_ID_OUT0_INFO, (UINT32)&tse_out_buf);
	tse_start(TRUE, TSE_MODE_TSDEMUX);

	if (test_size != tse_getConfig(TSDEMUX_CFG_ID_OUT0_TOTAL_LEN)) {
		DBG_ERR("size0 NG (0x%.8x, 0x%.8x)\r\n", test_size, tse_getConfig(TSDEMUX_CFG_ID_OUT0_TOTAL_LEN));
		ret = -1;
		goto err;
	} else {
		DBG_DUMP("size0 OK (0x%.8x, 0x%.8x)\r\n", test_size, tse_getConfig(TSDEMUX_CFG_ID_OUT0_TOTAL_LEN));
	}
	ret = nvt_tse_demux_verify(tse_out_buf.addr, test_size, 0);

	tse_close();
err:
	if (in_handle) {
		nvtmem_release_buffer(in_handle);
	}

	if (out_handle) {
		nvtmem_release_buffer(out_handle);
	}

	DBG_DUMP("mux test exit...done(%d)\n", ret);
	return ret;
}
#endif
