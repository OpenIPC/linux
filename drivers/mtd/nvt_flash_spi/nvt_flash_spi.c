/*
    @file       nvt_flash_spi.c

    @brief      Low layer driver which will access to FLASH SPI controller

    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

*/
#include "nvt_flash_spi_int.h"
#include "nvt_flash_spi_reg.h"
/*
    Configure nand host transfer register

    This function configure controller transfer related register

    @param[in]  trans_param      Setup transfer configuration
    @param[in]  page_count       Transfer page count
    @param[in]  length          Transfer length
    @param[in]  multi_page_select multi page / multi spare / single page

    @return void
*/
void nand_host_setup_transfer(struct drv_nand_dev_info *info, \
				struct smc_setup_trans *trans_param, \
				uint32_t page_count, \
				uint32_t length, \
				uint32_t multi_page_select)
{
	const struct nvt_nand_flash *f = info->flash_info;
	uint32_t value3 = 0;
	uint32_t value = 0;
	uint32_t value1 = 0;

	//#NT#2012/09/05#Steven Wang -begin
	//#NT#0039432,Temp disable complete interrupt mask
	value3 = NAND_GETREG(info, NAND_INTMASK_REG_OFS);
	value3 &= ~NAND_COMP_INTREN;
	NAND_SETREG(info, NAND_INTMASK_REG_OFS, value3);
	//#NT#2012/09/05#Steven Wang -end

	if (page_count > 0)
	{
		NAND_SETREG(info, NAND_PAGENUM_REG_OFS, page_count);
		value = multi_page_select;
	}
	//#NT#2012/09/05#Steven Wang -begin
	//#NT#0039432, Set single or multipage first
	value1 = NAND_GETREG(info, NAND_CTRL0_REG_OFS);
	NAND_SETREG(info, NAND_CTRL0_REG_OFS, value);
	//#NT#2012/09/05#Steven Wang -end

	// Set column address
	NAND_SETREG(info, NAND_COLADDR_REG_OFS, trans_param->col_addr);

	// Set row address
	NAND_SETREG(info, NAND_ROWADDR_REG_OFS, trans_param->row_addr);

	// Set data length
	NAND_SETREG(info, NAND_DATALEN_REG_OFS, length);

	//#NT#2010/02/09#Steven Wang -begin
	//#NT#Only for NT96460
	if((value1 & _PROTECT1_EN) == _PROTECT1_EN)
	{
		value |= _PROTECT1_EN;
	}
	if((value1 & _PROTECT2_EN) == _PROTECT2_EN)
	{
		value |= _PROTECT2_EN;
	}


	value |= (trans_param->type | trans_param->cs);


	if(trans_param->fifo_dir == _FIFO_READ)
		value |= _NAND_WP_EN;

	NAND_SETREG(info, NAND_CTRL0_REG_OFS, value);

	//#NT#2012/09/05#Steven Wang -begin
	//#NT#1. Clear status
	//#NT#2. Re enable complete interrupt mask
	//#NT#0039432
	value3 = NAND_GETREG(info, NAND_CTRL_STS_REG_OFS);
	value3 |= NAND_COMP_STS;
	NAND_SETREG(info, NAND_CTRL_STS_REG_OFS, value3);
	//#NT#2012/09/05#Steven Wang -end

	// Set control0 register
	if (length && ((length > GET_NAND_SPARE_SIZE(f)) || \
					(length == 8) || (length == 4) || \
					(multi_page_select == _MULTI_SPARE))) {
		NAND_SETREG(info, NAND_FIFO_CTRL_REG_OFS, \
			(trans_param->fifo_dir | trans_param->trans_mode));

		NAND_SETREG(info, NAND_FIFO_CTRL_REG_OFS, (_FIFO_EN | \
							trans_param->fifo_dir | \
							trans_param->trans_mode));
	}


	if ((page_count == 0) && (length == 0x40)) {
		NAND_SETREG(info, NAND_FIFO_CTRL_REG_OFS, \
			(trans_param->fifo_dir | trans_param->trans_mode));

		NAND_GETREG(info, NAND_FIFO_CTRL_REG_OFS);

		NAND_SETREG(info, NAND_FIFO_CTRL_REG_OFS, (_FIFO_EN | \
							trans_param->fifo_dir | \
							trans_param->trans_mode));
	}
	//#NT#0039432, re enable comp interrupt
	value3 = NAND_GETREG(info, NAND_INTMASK_REG_OFS);
	value3 |= NAND_COMP_INTREN;
	NAND_SETREG(info, NAND_INTMASK_REG_OFS, value3);
}

/*
    Configure SM transfer command

    Command send to NAND

    @param[in]  command     nand command
    @param[in]  tmout_en    time out enable / disable

    @return void
*/
void nand_host_send_command(struct drv_nand_dev_info *info, uint32_t command,
							uint32_t tmout_en)
{
	union NAND_CTRL0_REG reg_ctrl0;

	// Set command
	NAND_SETREG(info, NAND_COMMAND_REG_OFS, command);
	reg_ctrl0.reg = NAND_GETREG(info, NAND_CTRL0_REG_OFS);
	reg_ctrl0.bit.oper_en = 1;
	reg_ctrl0.bit.timeout_en = tmout_en;

	NAND_SETREG(info, NAND_CTRL0_REG_OFS, reg_ctrl0.reg);
}


/*
    NAND controller host receive data routing

    After read operation related command issued, called this function to reveive data

    @param[in]  buffer      buffer receive to
                length      receive buffer length
                trans_mode   PIO/DMA mode usage

    @return
        - @b E_OK           receive data success
        - @b E_SYS          error status
*/
int nand_host_receive_data(struct drv_nand_dev_info *info, uint8_t *buffer,
					uint32_t length, uint32_t trans_mode)
{
	int32_t index;
	uint8_t *eccbuf;

	eccbuf = buffer;

	if(trans_mode == _PIO_MODE) {
		length = ((length+3)/4) * 4;

		while(length) {
			if(length < 64) {
				if((NAND_GETREG(info, NAND_FIFO_STS_REG_OFS) & _FIFO_CNT_MASK) == (length/4)) {
					for (index = length; index > 0; index -= 4) {
						*((uint32_t *)buffer) = NAND_GETREG(info, NAND_DATAPORT_REG_OFS);
						buffer += 4;
						length -= 4;
					}
				}
			} else {
				if(NAND_GETREG(info, NAND_FIFO_STS_REG_OFS) & _FIFO_FULL) {
					for (index = 64; index > 0; index -= 4) {
						*((uint32_t *)buffer) = NAND_GETREG(info, NAND_DATAPORT_REG_OFS);
						buffer += 4;
						length -= 4;
					}
				}
			}
		}
	}
	return nand_cmd_wait_complete(info);
}


/*
    NAND controller host transmit data routing

    After write operation related command issued, called this function to transmit data

    @param[in]  buffer      buffer transmit from
                length      transmit buffer length
                trans_mode   PIO/DMA mode usage

    @return
        - @b E_OK           transmit data success
        - @b E_SYS          error status
*/
int nand_host_transmit_data(struct drv_nand_dev_info *info, uint8_t *buffer, uint32_t length, uint32_t trans_mode)
{
	int32_t index;
	uint8_t *eccbuf;

	eccbuf = buffer;

	if(trans_mode == _PIO_MODE) {
		length = ((length+3)/4) * 4;

		while(length) {
			if(length < 64) {
				if(NAND_GETREG(info, NAND_FIFO_STS_REG_OFS) & _FIFO_EMPTY) {
					for (index = length; index > 0; index -= 4) {
						NAND_SETREG(info, NAND_DATAPORT_REG_OFS, *((uint32_t *)buffer));
						buffer += 4;
						length -= 4;
					}
				}
			} else {
				if(NAND_GETREG(info, NAND_FIFO_STS_REG_OFS) & _FIFO_EMPTY)
				{
					for (index = 64; index > 0; index -= 4) {
						NAND_SETREG(info, NAND_DATAPORT_REG_OFS, *((uint32_t *)buffer));
						buffer += 4;
						length -= 4;
					}
				}
			}
		}
	}
	return nand_cmd_wait_complete(info);
}

/*
    Enable / Disable FIFO

    Enable and Disable FIFO of NAND controller

    @param[in]  en     enable / disable

    @return void
*/
void nand_host_set_fifo_enable(struct drv_nand_dev_info *info, uint32_t en)
{
	union NAND_FIFO_CTRL_REG reg_fifo_ctrl;

	reg_fifo_ctrl.reg = NAND_GETREG(info, NAND_FIFO_CTRL_REG_OFS);

	reg_fifo_ctrl.bit.fifo_en = en;

	NAND_SETREG(info, NAND_FIFO_CTRL_REG_OFS, reg_fifo_ctrl.reg);

	if(en == FALSE)
		while((NAND_GETREG(info, NAND_FIFO_CTRL_REG_OFS) & _FIFO_EN) != 0);
}


/*
    Set NAND controller error correction SRAM access right(for CPU or NAND controller)

    Set register to switch error information SRAM access right to CPU or NAND controller

    @param[in]  bcpu_Acc
            - @b TRUE       switch error information sram area access right to CPU
            - @b FALSE      Switch error information sram area access right to NAND controller access

    @return
        - void
*/
void nand_host_set_cpu_access_err(struct drv_nand_dev_info *info, uint32_t bcpu_Acc)
{
	union NAND_SRAM_ACCESS_REG  reg_sram_acc = {0x00000000};
	uint8_t i;

	reg_sram_acc.reg = NAND_GETREG(info, NAND_SRAM_ACCESS_REG_OFS);
	reg_sram_acc.bit.error_acc = bcpu_Acc;

	NAND_SETREG(info, NAND_SRAM_ACCESS_REG_OFS,reg_sram_acc.reg);

	//dummy read to delay 200ns for SRAM ready
	if(bcpu_Acc == TRUE) {
	for(i = 0; i < SRAMRDY_DELAY; i++)
		reg_sram_acc.reg = NAND_GETREG(info, NAND_SRAM_ACCESS_REG_OFS);
	}
}


/*
    Set NAND controller spare SRAM area access right(for CPU or NAND controller)

    Set register to switch spare area SRAM access right to CPU or NAND controller

    @param[in]  bcpu_Acc
            - @b TRUE       switch spare sram area access right to CPU
            - @b FALSE      switch spare sram area access right to NAND controller

    @return
        - void
*/
void nand_host_set_cpu_access_spare(struct drv_nand_dev_info *info,
							uint32_t bcpu_Acc)
{
	union NAND_SRAM_ACCESS_REG  reg_sram_acc = {0x00000000};
	uint8_t i;

	reg_sram_acc.reg = NAND_GETREG(info, NAND_SRAM_ACCESS_REG_OFS);
	reg_sram_acc.bit.spare_acc = bcpu_Acc;

	NAND_SETREG(info, NAND_SRAM_ACCESS_REG_OFS,reg_sram_acc.reg);

	//dummy read to delay 200ns for SRAM ready
	if(bcpu_Acc == TRUE) {
		for(i = 0; i < SRAMRDY_DELAY; i++)
			reg_sram_acc.reg = NAND_GETREG(info, NAND_SRAM_ACCESS_REG_OFS);
	}
}

/*
    NAND Reed Solomon ECC correction routing

    ECC correction flow by using reed solomon method,
    including correct error in spare area first 6 bytes

    @param[in]  Buf         Buffer to correct by RS correct routing
    @param[in]  section     section to be corrected

    @return
            - @b E_OK       success
            - @b E_CTX      uncorrect error
*/
int nand_host_correct_reedsolomon_ecc(struct drv_nand_dev_info *info,
					uint8_t * Buf, uint32_t section)
{
	int32_t ret = 0;
	uint32_t i, j, shifts;
	uint32_t hrdata, rs_sts, rs_err, err_reg, err_addr, err_bit, mask_bit;

	nand_host_set_cpu_access_err(info, TRUE);

	rs_sts = NAND_GETREG(info, NAND_RSERR_STS0_REG_OFS);

	for(i = 0; i < section; i++) {
		shifts = 4 * i;
		rs_err = (rs_sts >> shifts) & 0x7;
		if(rs_err == 0) {
			continue;
		} else if(rs_err == 5) {
			ret = E_CTX;
			continue;
		}

		for(j = 0; j < rs_err; j++) {
			err_reg = NAND_SEC0_EADDR0_REG_OFS + (16 * i) + (4 * j);

			hrdata = NAND_GETREG(info, err_reg);
			err_addr = hrdata & 0xFFFF;

			if(err_addr >= _512BYTE)
			{
				nand_host_set_cpu_access_spare(info, TRUE);
				err_addr -= _512BYTE;
				err_addr += (uint32_t)((info->mmio_base + NAND_SPARE00_REG_OFS) + (i * 16));
			}
			err_bit = (hrdata >> 16) & 0xFF;

			mask_bit = (err_addr & 0x03) * 8;

			if((hrdata & 0xFFFF) >= _512BYTE) {
				* (volatile uint32_t *) (err_addr & 0xFFFFFFFC) ^= (err_bit << mask_bit);
				nand_host_set_cpu_access_spare(info, FALSE);
			} else
				Buf[err_addr+ i*_512BYTE] ^= (err_bit);   // correct bit
		}
	}

	nand_host_set_cpu_access_err(info, FALSE);

	return ret;
}

/*
    NAND secondary ECC correction routing

    ECC correction flow by using secondary ECC method,

    @param[in]  Buf         Buffer to correct by RS correct routing
    @param[in]  section     section to be corrected

    @return
            - @b E_OK       success
            - @b E_CTX      uncorrect error
*/
int nand_host_correct_secondary_ecc(struct drv_nand_dev_info *info,
					uint32_t section)
{
	int32_t ret = E_OK;
	uint32_t i, shifts;
	uint32_t hrdata, ham_sts, ham_err, err_reg, err_addr, err_bit;
	uint32_t tmp_cs;
	union NAND_CTRL0_REG    nand_ctrl0;
	union NAND_MODULE0_REG  nand_module_cfg;

	uint32_t mask_bit;

	nand_host_set_cpu_access_err(info, TRUE);

	ham_sts = NAND_GETREG(info, NAND_SECONDARY_ECC_STS_REG_OFS);

	nand_ctrl0.reg = NAND_GETREG(info, NAND_CTRL0_REG_OFS);

	tmp_cs = nand_ctrl0.bit.chip_en;

	nand_module_cfg.reg = NAND_GETREG(info, tmp_cs * 4);

	for(i = 0; i < section; i++) {   // 8 sections
		shifts = 2 * i;
		ham_err = (ham_sts >> shifts) & 0x3;

		if(ham_err == 0) {
			continue;
		} else if(ham_err == 2) {
			ret = E_CTX;
			continue;
		} else if(ham_err == 3) {
			ret = E_CTX;
			continue;
		}
		err_reg = NAND_SEC0_EADDR0_REG_OFS + (16 * i) + 4;

		hrdata = NAND_GETREG(info, err_reg);

		err_addr = hrdata & _ECC_SEC0_SECONDARY_ERRBYTE;
		err_bit = (hrdata & _ECC_SEC0_SECONDARY_ERRBIT)>>_ECC_SEC0_SECONDARY_BITSFT;

		if(err_addr >= _512BYTE) {
			nand_host_set_cpu_access_spare(info, TRUE);
			err_addr -= _512BYTE;
			err_addr += (uint32_t)(info->mmio_base + NAND_SPARE00_REG_OFS + i * 16);
			mask_bit = (err_addr & 0x03) * 8;
			* (volatile uint32_t *) (err_addr & 0xFFFFFFFC) ^= ((1<<err_bit) << mask_bit);

			nand_host_set_cpu_access_spare(info, FALSE);
		}
		else {
			pr_info("Secondary ECC should occurred > section size[0x%08x]\r\n", _512BYTE);
			ret = E_SYS;
			break;
		}
	}
	nand_host_set_cpu_access_err(info, FALSE);

	return ret;
}

int nand_host_set_nand_type(struct drv_nand_dev_info *info, NAND_TYPE_SEL nand_type)
{
	union NAND_MODULE0_REG  reg_module0 = {0};
	int ret = E_OK;

	reg_module0.reg = NAND_GETREG(info, NAND_MODULE0_REG_OFS);
	switch(nand_type)
	{
	case NANDCTRL_ONFI_NAND_TYPE:
	reg_module0.bit.nand_type = 0;
	reg_module0.bit.spi_flash_type = 0;
	break;

	case NANDCTRL_SPI_NAND_TYPE:
	reg_module0.bit.nand_type = 1;
	reg_module0.bit.spi_flash_type = 0;
	reg_module0.bit.col_addr = NAND_1COL_ADDR;
	reg_module0.bit.row_addr = NAND_1ROW_ADDR;
	break;

	case NANDCTRL_SPI_NOR_TYPE:
	reg_module0.bit.nand_type = 1;
	reg_module0.bit.spi_flash_type = 1;
	reg_module0.bit.col_addr = NAND_1COL_ADDR;
	reg_module0.bit.row_addr = NAND_1ROW_ADDR;
	break;

	default:
	return E_NOSPT;
	break;
	}

	NAND_SETREG(info, NAND_MODULE0_REG_OFS, reg_module0.reg);

	return ret;
}

void nand_dll_reset(struct drv_nand_dev_info *info)
{
	union NAND_DLL_PHASE_DLY_REG1 reg_dll_ctrl1;

	reg_dll_ctrl1.reg = NAND_GETREG(info, NAND_DLL_PHASE_DLY_REG1_OFS);
	reg_dll_ctrl1.bit.phy_sw_reset = 1;
	NAND_SETREG(info, NAND_DLL_PHASE_DLY_REG1_OFS, reg_dll_ctrl1.reg);

	do {
		reg_dll_ctrl1.reg = NAND_GETREG(info, NAND_DLL_PHASE_DLY_REG1_OFS);
	} while (reg_dll_ctrl1.bit.phy_sw_reset == 1);

}

void nand_phy_config(struct drv_nand_dev_info *info)
{
	union NAND_DLL_PHASE_DLY_REG1 reg_phy_dly;
	union NAND_DLL_PHASE_DLY_REG2 reg_phy_dly2;
	union NAND_MODULE0_REG reg_module0;

	reg_phy_dly.reg = NAND_GETREG(info, NAND_DLL_PHASE_DLY_REG1_OFS);
	reg_phy_dly2.reg = NAND_GETREG(info, NAND_DLL_PHASE_DLY_REG2_OFS);
	reg_module0.reg = NAND_GETREG(info, NAND_MODULE0_REG_OFS);

	reg_phy_dly.bit.phy_sampclk_inv = 1;

	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (info->flash_freq > 96000000) {
			if (reg_module0.bit.spi_flash_type) {
				reg_phy_dly.bit.phy_src_clk_sel = 1;
				reg_phy_dly2.bit.indly_sel = 0x3F;
			} else {
				reg_phy_dly.bit.phy_src_clk_sel = 0;
				reg_phy_dly2.bit.indly_sel = 0x20;
			}
		} else {
			reg_phy_dly.bit.phy_src_clk_sel = 0;
			reg_phy_dly2.bit.indly_sel = 0x0;
		}
	} else if (nvt_get_chip_id() == CHIP_NA51089) {
	    reg_phy_dly.bit.phy_src_clk_sel = 0;
		reg_phy_dly.bit.phy_pad_clk_sel = 1;
		reg_phy_dly2.bit.indly_sel = 0x5;
	} else {
		reg_phy_dly.bit.phy_src_clk_sel = 0;
		reg_phy_dly.bit.phy_pad_clk_sel = 1;
		reg_phy_dly2.bit.indly_sel = 0xF;
	}

	NAND_SETREG(info, NAND_DLL_PHASE_DLY_REG2_OFS, reg_phy_dly2.reg);
	NAND_SETREG(info, NAND_DLL_PHASE_DLY_REG1_OFS, reg_phy_dly.reg);
}

int nand_host_setup_page_size(struct drv_nand_dev_info *info, NAND_PAGE_SIZE page_size)
{
	union NAND_MODULE0_REG  reg_module0 = {0};

	reg_module0.reg = NAND_GETREG(info, NAND_MODULE0_REG_OFS);

	if(page_size == NAND_PAGE_SIZE_512 || page_size == NAND_PAGE_SIZE_2048 || page_size == NAND_PAGE_SIZE_4096)
	{
		reg_module0.bit.page_size = page_size;
		NAND_SETREG(info, NAND_MODULE0_REG_OFS, reg_module0.reg);
		return E_OK;
	}
	else
		return E_SYS;
}


/*
    Configure nand host row and column address cycle

    This function configure controller row & column cycle

    @param[in]  uiCS		chip select
    @param[in]  row 		row     cycle
    @param[in]  uiColumn	column  cycle

    @return
        - @b E_SYS      Status fail
        - @b E_OK       Operation success

*/
int nand_host_setup_address_cycle(struct drv_nand_dev_info *info,
		NAND_ADDRESS_CYCLE_CNT row, NAND_ADDRESS_CYCLE_CNT col)
{
	union NAND_MODULE0_REG  reg_module0 = {0};

	reg_module0.reg = NAND_GETREG(info, NAND_MODULE0_REG_OFS);

	if(col > NAND_3_ADDRESS_CYCLE_CNT && col != NAND_NOT_CONFIGED_ADDRESS_CYCLE_CNT)
		return E_SYS;

	if(col != NAND_NOT_CONFIGED_ADDRESS_CYCLE_CNT)
		reg_module0.bit.col_addr = col;

	if(row != NAND_NOT_CONFIGED_ADDRESS_CYCLE_CNT)
		reg_module0.bit.row_addr = row;

	NAND_SETREG(info, NAND_MODULE0_REG_OFS, reg_module0.reg);

	return E_OK;
}

/*
    NAND controller enable polling bit match operation

    Enable bit value compare function.
    After invoke spi_enBitMatch(), it will send uiCmd to SPI device
    and continously read data from SPI device.
    Once bit position specified by uiBitPosition of read data becomes bWaitValue,
    SPI module will stop checking and raise a interrupt.
    Caller of spi_enBitMatch() can use spi_waitBitMatch() to wait this checking complete.

    @param[in] status_value    The check value
    @param[in] status_mask     Indicates which bit of status value you want to check. 1 for comparison and 0 for not comparison

    @return
        - @b E_OK: success
        - @b Else: fail
*/
int nand_host_setup_status_check_bit(struct drv_nand_dev_info *info, u8 status_mask, u8 status_value)
{
	union NAND_STATUS_CHECK_REG reg_status_check = {0};

	reg_status_check.bit.status_value = status_value;
	reg_status_check.bit.status_mask = status_mask;

	NAND_SETREG(info, NAND_STATUS_CHECK_REG_OFS, reg_status_check.reg);
	return E_OK;
}

/*
    Configure nand host page size

    This function configure controller of SPI NAND page size

    @param[OUT]
    @return
        - @b_MULTI_PAGE
        - @b_SINGLE_PAGE

*/
int nand_host_set_spi_io(struct drv_nand_dev_info *info,
		NAND_SPI_CS_POLARITY cs_pol, NAND_SPI_BUS_WIDTH bus_width,
			NAND_SPI_IO_ORDER io_order)
{
	union NAND_SPI_CFG_REG  reg_cfg = {0};

	reg_cfg.reg = NAND_GETREG(info, NAND_SPI_CFG_REG_OFS);

	reg_cfg.bit.spi_cs_pol    = cs_pol;
	reg_cfg.bit.spi_bs_width  = bus_width;
	reg_cfg.bit.spi_io_order  = io_order;
	reg_cfg.bit.spi_pull_wphld  = 0x1;

	NAND_SETREG(info, NAND_SPI_CFG_REG_OFS, reg_cfg.reg);
	return E_OK;
}

/*
    Configure nand host page size

    This function configure controller of SPI NAND page size

    @param[OUT]
    @return
        - @b_MULTI_PAGE
        - @b_SINGLE_PAGE

*/
uint32_t nand_host_get_multi_page_select(struct drv_nand_dev_info *info)
{
	union NAND_CTRL0_REG  reg_ctrl = {0};

	reg_ctrl.reg = NAND_GETREG(info, NAND_CTRL0_REG_OFS);
	return (reg_ctrl.reg & 0x00030000);
}

/*
    Configure nand host chip select manual mode

    Configure nand host chip select manual mode

     @param[in] level    CS (chip select) level
            - NAND_SPI_CS_LOW    : CS set low
            - NAND_SPI_CS_HIGH   : CS set high

*/
int nand_host_set_cs_active(struct drv_nand_dev_info *info, \
				NAND_SPI_CS_LEVEL level)
{
	union NAND_SPI_CFG_REG  cfg_reg = {0};

	cfg_reg.reg = NAND_GETREG(info, NAND_SPI_CFG_REG_OFS);
	cfg_reg.bit.spi_nand_cs = level;

	NAND_SETREG(info, NAND_SPI_CFG_REG_OFS, cfg_reg.reg);
	return E_OK;
}

/*
    Configure nand host chip select manual mode

    Configure nand host chip select manual mode

     @param[in] mode    manual mode or auto mode
            - NAND_SPI_CS_AUTO_MODE     : manual mode(CS configure by user)
            - NAND_SPI_CS_MANUAL_MODE   : auto mode(CS configure by controller)

*/
int nand_host_config_cs_opmode(struct drv_nand_dev_info *info, \
				NAND_SPI_CS_OP_MODE mode)
{
	union NAND_SPI_CFG_REG  cfg_reg = {0};

	cfg_reg.reg = NAND_GETREG(info, NAND_SPI_CFG_REG_OFS);
	cfg_reg.bit.spi_operation_mode = mode;

	NAND_SETREG(info, NAND_SPI_CFG_REG_OFS, cfg_reg.reg);
	return E_OK;
}

/*
    Configure nand host timing 2 configuration

    Configure nand host timing 2 configuration

     @param[in] bCSLevel    CS (chip select) level
            - NAND_SPI_CS_LOW    : CS set low
            - NAND_SPI_CS_HIGH   : CS set high

*/
void nand_host_settiming2(struct drv_nand_dev_info *info, u32 time)
{
	union NAND_TIME2_REG  time_reg = {0};

	time_reg.reg = time;

	if (time_reg.bit.tshch < 0x4) {
		pr_warn("SPI NAND tshch need >= 0x4, force config as 0x4\n");
		time_reg.bit.tshch = 0x4;
	}

	NAND_SETREG(info, NAND_TIME2_REG_OFS, time_reg.reg);
}
