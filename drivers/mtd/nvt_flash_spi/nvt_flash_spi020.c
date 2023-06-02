/*
	@file       nvt_flash_spi.c
	@ingroup    mIStgNAND

	@brief      NAND low layer driver which will access to NAND controller

	@note       Nothing

	Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.

*/
#include "nvt_flash_spi020_int.h"
#include "nvt_flash_spi020_reg.h"

/*
#define NAND_CMP_CHKSUM
#define NAND_USE_DMA
*/

void nand_hw_init(struct drv_nand_dev_info *info)
{
	union INT_CTRL_REG int_ctrl_reg;

	int_ctrl_reg.reg = NAND_GETREG(info, INT_CTRL_REG_OFS);
	int_ctrl_reg.bit.INT_EN = 1;
	NAND_SETREG(info, INT_CTRL_REG_OFS, int_ctrl_reg.reg);
}

/*
	nand_cmd_wait_complete

	Check NAND operation command complete.

	@return
		- @b E_SYS      Status fail
		- @b E_OK       Operation success
*/
int nand_cmd_wait_complete(struct drv_nand_dev_info *info)
{
	u32 ret = 0;
	// Use interrupt to wait command.
	ret = wait_for_completion_timeout(&info->cmd_complete, 4*HZ);

	if (ret == 0) {
		pr_err("wait timeout with status\n");
		return -EINTR;
	}

	return E_OK;
}

#ifdef NAND_CMP_CHKSUM
static UINT32 nand_gen_checksum(PFTSPI_TRANS p_trans)
{
	UINT32 chksum;
	UINT32 i;
	UINT32 remain;
	UINT8* p_buf;

	chksum = 0;
	p_buf = (UINT8*)p_trans->data_buf;
	remain = p_trans->data_len;
	for (i=0; i<p_trans->data_len; i+=4) {
		UINT32 slice;
		UINT32 j;
		UINT32 data = 0;

		if (remain > 4) {
			slice = 4;
		} else {
			slice = remain;
		}

		for (j=0; j<slice; j++) {
			data |= p_buf[i+j] << (j*8);
		}
		chksum += data;

		remain -= slice;
	}

	return chksum;
}
#endif

static ER nand_host_data_process(struct drv_nand_dev_info *info, PFTSPI_TRANS p_trans)
{
#ifdef NAND_USE_DMA
	if (p_trans->data_len) {
		FLGPTN flag = 0;
		union INT_CTRL_REG int_ctrl_reg;

		int_ctrl_reg.reg = NAND_GETREG(info, INT_CTRL_REG_OFS);
		int_ctrl_reg.bit.DMA_EN = 1;
		NAND_SETREG(info, INT_CTRL_REG_OFS, int_ctrl_reg.reg);

		dmac030_open(nand_dma_isr);

		dmac030_set_config(DMAC030_CH_0, DMAC030_CONFIG_SRC_WIDTH, DMAC030_WIDTH_32BITS);
		dmac030_set_config(DMAC030_CH_0, DMAC030_CONFIG_DES_WIDTH, DMAC030_WIDTH_32BITS);

		if (p_trans->data_dir == FIFO_DIR_READ) {
			vos_cpu_dcache_sync((VOS_ADDR)p_trans->data_buf, p_trans->data_len, VOS_DMA_FROM_DEVICE);
			dmac030_set_SPI_rx_handshake_mode(DMAC030_CH_0);
			dmac030_trigger(DMAC030_CH_0,
			IOADDR_NAND_REG_BASE + FLASH_DATA_PORT_REG_OFS,
			p_trans->data_buf,
			(p_trans->data_len+3)/4);
		} else {
			vos_cpu_dcache_sync((VOS_ADDR)p_trans->data_buf, p_trans->data_len, VOS_DMA_TO_DEVICE);
			dmac030_set_SPI_tx_handshake_mode(DMAC030_CH_0);
			dmac030_trigger(DMAC030_CH_0, p_trans->data_buf, IOADDR_NAND_REG_BASE + FLASH_DATA_PORT_REG_OFS, (p_trans->data_len+3)/4);
		}

		// wait complete
		wai_flg(&flag, FLG_ID_NAND_DMA, FLGPTN_NAND, TWF_ORW | TWF_CLR);

		vos_cpu_dcache_sync((VOS_ADDR)p_trans->data_buf, p_trans->data_len, VOS_DMA_FROM_DEVICE);

		dmac030_close();
	}

#else
	// if PIO
	if (1) {
		// PIO
		if (p_trans->data_len) {
			INT32 slice;
			UINT8 *ptr = (UINT8*)p_trans->data_buf;
			UINT32 remain = p_trans->data_len;
			UINT32 max_fifo_fepth = MAX_FIFO_DEPTH;

			if (p_trans->data_dir == FIFO_DIR_READ) {
				while(remain) {
					INT32 i;

					slice = min(max_fifo_fepth, remain);
					remain -= slice;

					// wait rx fifo ready
					while (1) {
						union FLASH_STATUS_REG status_reg;

						status_reg.reg = NAND_GETREG(info, FLASH_STATUS_REG_OFS);
						if (status_reg.bit.RX_FIFO_FULL) break;
					}

					// read out rx fifo
					while (slice > 0) {
						UINT32 data;
						INT32 bytes;

						data = NAND_GETREG(info, FLASH_DATA_PORT_REG_OFS);

						bytes = min((UINT32)slice, (UINT32)sizeof(UINT32));
						slice -= bytes;

						for (i=0; i<bytes; i++) {
							*ptr++ = data & 0xFF;
							data >>= 8;
						}
					}
				}
			} else {
				while(remain) {
					slice = min(max_fifo_fepth, remain);
					remain -= slice;

					// wait tx fifo empty
					while (1) {
						union FLASH_STATUS_REG status_reg;

						status_reg.reg = NAND_GETREG(info, FLASH_STATUS_REG_OFS);
						if (status_reg.bit.TX_FIFO_EMPTY) break;
					}

					// read out rx fifo
					while (slice > 0) {
						UINT32 data;

						data = ptr[0] | (ptr[1]<<8) | (ptr[2]<<16) | (ptr[3]<<24);
						ptr += 4;
						slice -= 4;

						NAND_SETREG(info, FLASH_DATA_PORT_REG_OFS, data);
					}
				}
			}
		}
	} else {
		// DMA: do nothing
	}
#endif

#ifdef NAND_CMP_CHKSUM
	if (p_trans->data_len) {
		if (p_trans->data_dir == FIFO_DIR_READ) {
			UINT32 hw_chksum;
			union CHKSUM_CTRL_REG chksum_ctrl_reg;

			chksum_ctrl_reg.reg = NAND_GETREG(info, CHKSUM_CTRL_REG_OFS);

			hw_chksum = NAND_GETREG(info, RFIFO_CHKSUM_REG_OFS);

			if (hw_chksum != nand_gen_checksum(p_trans)) {
				while (1);
			}

			chksum_ctrl_reg.bit.RFIFO_CHKSUM_CLR = 1;
			NAND_SETREG(info, CHKSUM_CTRL_REG_OFS, chksum_ctrl_reg.reg);
			while (chksum_ctrl_reg.bit.RFIFO_CHKSUM_CLR == 1) {
				chksum_ctrl_reg.reg = NAND_GETREG(info, CHKSUM_CTRL_REG_OFS);
			}
		} else {
			UINT32 hw_chksum;
			union CHKSUM_CTRL_REG chksum_ctrl_reg;

			chksum_ctrl_reg.reg = NAND_GETREG(info, CHKSUM_CTRL_REG_OFS);

			hw_chksum = NAND_GETREG(info, WFIFO_CHKSUM_REG_OFS);

			if (hw_chksum != nand_gen_checksum(p_trans)) {
				while (1);
			}

			chksum_ctrl_reg.bit.WFIFO_CHKSUM_CLR = 1;
			NAND_SETREG(CHKSUM_CTRL_REG_OFS, chksum_ctrl_reg.reg);
			while (chksum_ctrl_reg.bit.WFIFO_CHKSUM_CLR == 1) {
				chksum_ctrl_reg.reg = NAND_GETREG(info, CHKSUM_CTRL_REG_OFS);
			}
		}
	}
#endif

	return E_OK;
}


static UINT32 nand_host_get_flash_status(struct drv_nand_dev_info *info, PFTSPI_TRANS p_trans)
{
	union FLASH_READ_STS_REG sts_reg;

	sts_reg.reg = NAND_GETREG(info, FLASH_READ_STS_REG_OFS);

	return sts_reg.reg;
}

/*
	Configure nand host transfer register

	This function configure controller transfer related register

	@param[in]  transParam      Setup transfer configuration
	@param[in]  pageCount       Transfer page count
	@param[in]  length          Transfer length
	@param[in]  multiPageSelect multi page / multi spare / single page

	@return void
*/
int nand_host_issue_transfer(struct drv_nand_dev_info *info, PFTSPI_TRANS p_trans)
{
	union CMD_QUEUE_2ND_REG q2nd_reg = {0};
	union CMD_QUEUE_4TH_REG q4th_reg = {0};
	int ret;

	if (p_trans == NULL) {
		pr_err("%s: NULL input\r\n", __func__);
		return E_SYS;
	}

	if (p_trans->cs_sel >= FCTRL_CS_COUNT) {
		pr_err("%s: invalid CS select: %d\r\n", __func__, p_trans->cs_sel);
		return E_SYS;
	}

	q4th_reg.bit.CS_SEL = p_trans->cs_sel;
	q4th_reg.bit.INSTRUCTION_CODE = p_trans->command;
	q2nd_reg.bit.INSTRUCTION_LEN = 1;

	if (p_trans->io_mode >= FCTRL_IO_MODE_COUNT) {
		pr_err("%s: invalid IO mode select: %d\r\n", __func__, p_trans->io_mode);
		return E_SYS;
	}
	q4th_reg.bit.SPI_OPERATE_MODE = p_trans->io_mode;

	if ((p_trans->ctrl_mode == FCTRL_OP_NORMAL) &&
		(p_trans->data_dir == FIFO_DIR_READ) &&
		(p_trans->data_len != 0)) {
		if (p_trans->xdr_sel == FCTRL_DDR) {
			q4th_reg.bit.DTR_MODE = 1;
		}
	}

	if (p_trans->flash_addr_count) {
		NAND_SETREG(info, CMD_QUEUE_1ST_REG_OFS, p_trans->flash_addr);
	}
	q2nd_reg.bit.ADDR_LEN = p_trans->flash_addr_count;

	if (p_trans->is_conti_mode) {
		q4th_reg.bit.CONTI_MODE_CODE = p_trans->conti_mode;
		q2nd_reg.bit.CONTI_MODE_EN = 1;
	}

	if (p_trans->dummy_cyles > DUMMY_CYCLES_MAX) {
		pr_err("%s: max dummy cycles %d, but input %d\r\n", __func__,
			DUMMY_CYCLES_MAX, p_trans->dummy_cyles);
		return E_SYS;
	}
	q2nd_reg.bit.DUMMY_CYCLES = p_trans->dummy_cyles;

	q4th_reg.bit.WRITE_EN = 1;
	if (p_trans->data_len && (p_trans->data_dir == FIFO_DIR_READ)) {
		q4th_reg.bit.WRITE_EN = 0;
	}

	switch (p_trans->ctrl_mode) {
	case FCTRL_OP_NORMAL:
		NAND_SETREG(info, CMD_QUEUE_3RD_REG_OFS, p_trans->data_len);
		break;
	case FCTRL_OP_READ_STATUS:              // by sw
		q4th_reg.bit.RD_STS_MODE = RD_STS_MODE_BY_SW;
	case FCTRL_OP_POLLING_STATUS:           // by hw
		q4th_reg.bit.RD_STS_EN = 1;
		q4th_reg.bit.WRITE_EN = 0;
		// read status => force data counter to zero
		NAND_SETREG(info, CMD_QUEUE_3RD_REG_OFS, 0);
		break;
	default:
		pr_err("%s: invalid ctrl mode: 0x%x\r\n", __func__, p_trans->ctrl_mode);
		return E_SYS;
	}

#ifdef NAND_CMP_CHKSUM
	if (p_trans->data_len) {
		union CHKSUM_CTRL_REG chksum_ctrl_reg;

		chksum_ctrl_reg.reg = NAND_GETREG(info, CHKSUM_CTRL_REG_OFS);

		if (p_trans->data_dir == FIFO_DIR_READ) {
		chksum_ctrl_reg.bit.RFIFO_CHKSUM_EN = 1;
			pr_err("%s: R 0x%x\r\n", __func__, (unsigned int)chksum_ctrl_reg.reg);
		} else {
		chksum_ctrl_reg.bit.WFIFO_CHKSUM_EN = 1;
			pr_err("%s: W 0x%x\r\n", __func__, (unsigned int)chksum_ctrl_reg.reg);
		}
		NAND_SETREG(info, CHKSUM_CTRL_REG_OFS, chksum_ctrl_reg.reg);

		chksum_ctrl_reg.reg = NAND_GETREG(CHKSUM_CTRL_REG_OFS);
		pr_err("%s: read 0x%x\r\n", __func__, (unsigned int)chksum_ctrl_reg.reg);
	}
#endif

	// if DMA, must call DMA driver here (TBD)

	NAND_SETREG(info, CMD_QUEUE_2ND_REG_OFS, q2nd_reg.reg);
	NAND_SETREG(info, CMD_QUEUE_4TH_REG_OFS, q4th_reg.reg);   // trigger

	nand_host_data_process(info, p_trans);

	// wait complete
	ret = nand_cmd_wait_complete(info);
	if (ret)
		return E_SYS;

	if (p_trans->p_status) {
		*p_trans->p_status = nand_host_get_flash_status(info, p_trans);
	}

	return E_OK;
}