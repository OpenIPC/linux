#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <mach/drvdump.h>
#include <plat/pad.h>
#include "nvt_flash_spi020_reg.h"
#include "nvt_flash_spi020_int.h"

static struct drv_nand_dev_info drvdump_info;

/*
	Unlock SPI NAND block protect
*/
static ER spi_nand_unlock_BP(struct drv_nand_dev_info *info)
{
        FTSPI_TRANS_T trans = {0};
        UINT8 buffer[1];

	ER ret = E_OK;

	if (info->flash_info->spi_nand_status.block_unlocked == FALSE) {
		/*
		___|cmd(0x1F)|___|SR(0x01)(row byte0)|___|SR(0x0)(row byte1)|____
		*/
		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_NORMAL;
		trans.io_mode = FCTRL_IO_SERIAL;
		trans.command = SPINAND_CMD_SET_FEATURE;
		trans.flash_addr = SPINAND_FEATURE_BLOCK_LOCK;
		trans.flash_addr_count = 1;
		buffer[0] = 0x00;       // write 0 to flash register
		trans.data_buf = (UINT32)buffer;
		trans.data_dir = FIFO_DIR_WRITE;
		trans.data_len = 1;
		ret = nand_host_issue_transfer(info, &trans);

		if (ret == E_OK)
			info->flash_info->spi_nand_status.block_unlocked = TRUE;
	}

	return ret;
}

static ER spiNand_wait_status(struct drv_nand_dev_info *info, UINT32 feature, UINT32 sts_reg, UINT8 *p_status)
{
	ER ret = E_OK;
	do {
		FTSPI_TRANS_T trans = {0};
		UINT8 buffer[1];

		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_POLLING_STATUS;
		trans.io_mode = FCTRL_IO_SERIAL;
		trans.command = feature;
		trans.flash_addr = sts_reg;
		trans.flash_addr_count = 1;
		buffer[0] = 0x00;       // write 0 to flash register
		trans.data_buf = (UINT32)buffer;
		trans.data_dir = FIFO_DIR_READ;
		trans.data_len = 0;     // must be 0 when read status
		trans.p_status = p_status;
		ret = nand_host_issue_transfer(info, &trans);
	} while (0);

	return ret;
}


/*
	SPI NAND read page(SPI Single page read)

	@return
		- @b E_OK: success
		- @b E_OACV: already closed
		- @b E_NOMEM: read range exceed flash page
		- @b Else: fail
*/
static ER spiNand_readPage(struct drv_nand_dev_info *info, uint32_t uiRowAddr,
			uint32_t uiColAddr, uint8_t* pBuf, uint32_t uiBufSize)
{
	FTSPI_TRANS_T trans = {0};
	UINT8 buffer[1];
	UINT8 ucStatus;
	UINT32 uiPageCnt;
	UINT32 *pulBuf = (UINT32 *)pBuf;
	ER ret = E_OK;

	do {
		uiPageCnt = uiBufSize / info->flash_info->page_size;

		if (uiPageCnt == 1 && ((uiColAddr+uiBufSize) > (info->flash_info->page_size + info->flash_info->oob_size))) {
			pr_err("col addr 0x%08x, buf size 0x%08x exceed 0x%08x + 0x%08x\r\n",
				(int32_t)uiColAddr, (int32_t)uiBufSize, info->flash_info->page_size, info->flash_info->oob_size);
			ret = E_NOMEM;
			break;
		} else {
			if(uiBufSize % info->flash_info->page_size != 0 && uiColAddr != info->flash_info->page_size) {
				pr_err("uiBufSize[0x%08x] not multiple of page size[0x%08x]\r\n",
				uiBufSize, info->flash_info->page_size);
				ret = E_SYS;
				break;
			}
		}

		// Issue page read command
		memset(&trans, 0, sizeof(trans));
		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_NORMAL;
		trans.io_mode = FCTRL_IO_SERIAL;
		trans.command = SPINAND_CMD_PAGE_READ;
		trans.flash_addr  = uiRowAddr;
		trans.flash_addr_count = 3;
		buffer[0] = 0x00;       // write 0 to flash register
		trans.data_len = 0;
		trans.data_dir = FIFO_DIR_WRITE;
		trans.data_buf = (UINT32)buffer;
		ret = nand_host_issue_transfer(info, &trans);
		if (ret != E_OK) {
			break;
		}

		ret = spiNand_wait_status(info, SPINAND_CMD_GET_FEATURE, SPINAND_FEATURE_STATUS, &ucStatus);

		if (ret != E_OK) {
			pr_err("%s: wait status fail\r\n", __func__);
			return ret;
		}

		// Issue read from cache command
		memset(&trans, 0, sizeof(trans));
		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_NORMAL;
#ifndef CONFIG_FLASH_ONLY_DUAL
		trans.io_mode = FCTRL_IO_QUAD;
		trans.command = SPINAND_CMD_CACHE_READX4;
#else
		trans.io_mode = FCTRL_IO_DUAL;
		trans.command = SPINAND_CMD_CACHE_READX2;
#endif
		trans.dummy_cyles = 8;
		if(info->flash_info->plane_flags) {
			if((uiRowAddr & 0x40) == 0x40) {
				trans.flash_addr  = (uiColAddr | (1<<12));
			} else {
				trans.flash_addr  = uiColAddr;
			}
		} else {
			trans.flash_addr  = uiColAddr;
		}
		trans.flash_addr_count = 2;
		trans.data_len = uiBufSize;
		trans.data_dir = FIFO_DIR_READ;
		trans.data_buf = (UINT32)pulBuf;
		ret = nand_host_issue_transfer(info, &trans);
		if (ret != E_OK) {
			break;
		}
	} while (0);

	return ret;
}

/*
	SPI NAND program page

	@param[in] uiRowAddr        row address
	@param[in] uiColAddr        column address
	@param[in] pBuf             buffer to be programmed
	@param[in] uiBufSize        buffer size
	@param[in] pSpareBuf        buffer to be programmed on spare
	@param[in] uiSpareSize      buffer size of pSpareBuf

	@return
		- @b E_OK: success
		- @b E_OACV: already closed
		- @b E_NOMEM: program range exceed flash page
		- @b Else: fail
*/
static ER spiNand_programPage(struct drv_nand_dev_info *info, UINT32 uiRowAddr, UINT32 uiColAddr, UINT32 uiBufSize, UINT8* pSpareBuf, UINT32 uiSpareSize)
{
	FTSPI_TRANS_T trans = {0};
	UINT8 buffer[1];
	UINT8 ucStatus;
	UINT32 *pulBuf = (UINT32 *)(info->data_buff + uiColAddr);
	ER ret = E_OK;

#if defined(CONFIG_NVT_FW_UPDATE_LED) && !defined(CONFIG_NVT_PWM)
	led_set_gpio_high(OP_PROGRAM);
#endif
	do {
		ret = spi_nand_unlock_BP(info);
		if (ret != E_OK)
			break;

		// write enable
		// ___|cmd|___ = 0x07 command cycle only
		memset(&trans, 0, sizeof(trans));
		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_NORMAL;
		trans.io_mode = FCTRL_IO_SERIAL;
		trans.command = SPINAND_CMD_WEN;
		buffer[0] = 0x00;       // write 0 to flash register
		trans.data_buf = (UINT32)buffer;
		trans.data_dir = FIFO_DIR_WRITE;
		ret = nand_host_issue_transfer(info, &trans);
		if (ret != E_OK) {
			break;
		}

		// Issue program load command
		memset(&trans, 0, sizeof(trans));
		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_NORMAL;
#ifndef CONFIG_FLASH_ONLY_DUAL
		trans.io_mode = FCTRL_IO_QUAD;
		trans.command = SPINAND_CMD_PROGRAM_LOADX4;
#else
		trans.io_mode = FCTRL_IO_SERIAL;
		trans.command = SPINAND_CMD_PROGRAM_LOAD;
#endif
		if(info->flash_info->plane_flags) {
			if((uiRowAddr & 0x40) == 0x40) {
				trans.flash_addr  = (uiColAddr | (1<<12));
			} else {
				trans.flash_addr  = uiColAddr;
			}
		} else {
			trans.flash_addr  = uiColAddr;
		}
		trans.flash_addr_count = 2;
		trans.data_len = uiBufSize;
		trans.data_dir = FIFO_DIR_WRITE;
		trans.data_buf = (UINT32)pulBuf;
		ret = nand_host_issue_transfer(info, &trans);
		if (ret != E_OK) {
			break;
		}

		// Issue program execute command
		memset(&trans, 0, sizeof(trans));
		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_NORMAL;
		trans.io_mode = FCTRL_IO_SERIAL;
		trans.command = SPINAND_CMD_PROGRAM_EXE;
		trans.flash_addr = uiRowAddr;
		trans.flash_addr_count = 3;
		ret = nand_host_issue_transfer(info, &trans);
		if (ret != E_OK) {
			break;
		}

		spiNand_wait_status(info, SPINAND_CMD_GET_FEATURE, SPINAND_FEATURE_STATUS, &ucStatus);

		if (ucStatus & SPINAND_FEATURE_STATUS_ERASE_FAIL) {
			pr_err("erase/program block fail 0x%x\r\n", uiRowAddr);
			ret = E_SYS;
		}
	} while (0);

#if defined(CONFIG_NVT_FW_UPDATE_LED) && !defined(CONFIG_NVT_PWM)
	led_set_gpio_low();
#endif
	return ret;
}
/*
	Reset NAND flash

	Send reset command to NAND flash

	@return
		- @b E_SYS      Status fail
		- @b E_TMOUT    Controller timeout
		- @b E_OK       Operation success

	@note for nand_CmdComplete()
*/
int nand_cmd_reset(struct drv_nand_dev_info *info)
{
	FTSPI_TRANS_T trans = {0};

	trans.cs_sel = info->flash_info->chip_sel;
	trans.ctrl_mode = FCTRL_OP_NORMAL;
	trans.io_mode = FCTRL_IO_SERIAL;
	trans.command = SPINAND_CMD_RESET;
	trans.data_len = 0;
	trans.data_dir = FIFO_DIR_WRITE;

	return nand_host_issue_transfer(info, &trans);
}


/*
	Read status command

	Send read status command to NAND flash

	@return
		- @b E_SYS      Status fail
		- @b E_TMOUT    Controller timeout
		- @b E_OK       Operation success

	@note for nand_CmdComplete()
*/
int nand_cmd_read_status(struct drv_nand_dev_info *info, uint32_t set)
{
	int ret;
	uint32_t status = 0;
	uint32_t spi_cmd = 0;

	if (set == NAND_SPI_STS_FEATURE_1)
		spi_cmd = SPINAND_FEATURE_BLOCK_LOCK;
	else if (set == NAND_SPI_STS_FEATURE_2)
		spi_cmd = SPINAND_FEATURE_OPT;
	else if (set == NAND_SPI_STS_FEATURE_4)
		spi_cmd = SPINAND_FEATURE_STATUS_ECC_UNCORR_ERR;
	else
		spi_cmd = SPINAND_FEATURE_STATUS;

	do {
		FTSPI_TRANS_T trans = {0};
		UINT8 buffer[1];
		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_READ_STATUS;
		trans.io_mode = FCTRL_IO_SERIAL;
		trans.command = SPINAND_CMD_GET_FEATURE;
		trans.flash_addr = spi_cmd;
		trans.flash_addr_count = 1;
		buffer[0] = 0x00;       // write 0 to flash register
		trans.data_buf = (UINT32)buffer;
		trans.data_dir = FIFO_DIR_READ;
		trans.data_len = 0;     // must be 0 when read status
		trans.p_status = (void*)&status;
		ret = nand_host_issue_transfer(info, &trans);
	} while (0);

	if(ret == E_OK)
		memcpy(info->data_buff + info->buf_start, &status, 1);

	return status;
}


/*
	Write status command

	Send write status command to NAND flash

	@return
		- @b E_SYS      Status fail
		- @b E_TMOUT    Controller timeout
		- @b E_OK       Operation success

	@note for nand_CmdComplete()
*/
int nand_cmd_write_status(struct drv_nand_dev_info *info, u32 set, u32 status)
{
	int ret;
	uint32_t spi_cmd = 0;

	if (set == NAND_SPI_STS_FEATURE_1)
		spi_cmd = SPINAND_FEATURE_BLOCK_LOCK;
	else if (set == NAND_SPI_STS_FEATURE_2)
		spi_cmd = SPINAND_FEATURE_OPT;
	else
		spi_cmd = SPINAND_FEATURE_STATUS;

	do {
		FTSPI_TRANS_T trans = {0};
		UINT8 buffer[1];

		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_NORMAL;
		trans.io_mode = FCTRL_IO_SERIAL;
		trans.command = SPINAND_CMD_SET_FEATURE;
		// combine 1 byte address + 1 byte status to pesudo 2 byte address
		trans.flash_addr = (spi_cmd<<8) | status;
		trans.flash_addr_count = 2;
		buffer[0] = 0x00;       // write 0 to flash register
		trans.data_buf = (UINT32)buffer;
		trans.data_dir = FIFO_DIR_WRITE;
		trans.data_len = 0;     // must be 0 when read status
		ret = nand_host_issue_transfer(info, &trans);
	} while (0);

	return ret;
}

/*
	Erase block command.

	Issue erase block command of NAND controller
	@note block_num is the physical block number instead of the address.

	@param[in] block_num   which physical block number want to erase
	@return success or fail
		- @b E_OK:     Erase operation success
		- @b E_TMOUT   Time out
		- @b E_SYS:    Erase operation fail
*/
int nand_cmd_erase_block(struct drv_nand_dev_info *info, uint32_t block_address)
{
	FTSPI_TRANS_T trans = {0};
	UINT8 buffer[1];
	UINT8 ucStatus;
	ER ret = E_OK;

#if defined(CONFIG_NVT_FW_UPDATE_LED) && !defined(CONFIG_NVT_PWM)
	led_set_gpio_high(OP_ERASE);
#endif

	do {
		ret = spi_nand_unlock_BP(info);
		if (ret != E_OK)
			break;

		// write enable
		// ___|cmd|___ = 0x07 command cycle only
		memset(&trans, 0, sizeof(trans));
		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_NORMAL;
		trans.io_mode = FCTRL_IO_SERIAL;
		trans.command = SPINAND_CMD_WEN;
		buffer[0] = 0x00;       // write 0 to flash register
		trans.data_buf = (UINT32)buffer;
		trans.data_dir = FIFO_DIR_WRITE;
		ret = nand_host_issue_transfer(info, &trans);
		if (ret != E_OK) {
			break;
		}

		// Issue erase command
		memset(&trans, 0, sizeof(trans));
		trans.cs_sel = info->flash_info->chip_sel;
		trans.ctrl_mode = FCTRL_OP_NORMAL;
		trans.io_mode = FCTRL_IO_SERIAL;
		trans.command = SPINAND_CMD_BLK_ERASE;
		trans.flash_addr = block_address;
		trans.flash_addr_count = 3;
		trans.data_dir = FIFO_DIR_WRITE;
		ret = nand_host_issue_transfer(info, &trans);
		if (ret != E_OK) {
			break;
		}

		spiNand_wait_status(info, SPINAND_CMD_GET_FEATURE, SPINAND_FEATURE_STATUS, &ucStatus);

		if (ucStatus & SPINAND_FEATURE_STATUS_ERASE_FAIL) {
			pr_debug("Erase block fail 0x%08x\r\n", block_address);
			ret = E_SYS;
		}

		if(ucStatus & SPINAND_FEATURE_STATUS_ERASE_FAIL) {
			ret = E_SYS;
			break;
		}

	} while (0);

#if defined(CONFIG_NVT_FW_UPDATE_LED) && !defined(CONFIG_NVT_PWM)
	led_set_gpio_low();
#endif

	return ret;
}
/*
	Read device ID.

	@return Device ID
*/
int nand_cmd_read_id(uint8_t * card_id, struct drv_nand_dev_info *info)
{
	FTSPI_TRANS_T trans = {0};

	trans.cs_sel = info->flash_info->chip_sel;
	trans.ctrl_mode = FCTRL_OP_NORMAL;
	trans.io_mode = FCTRL_IO_SERIAL;
	trans.command = SPINAND_CMD_JEDECID;
	trans.flash_addr = 0;
	trans.flash_addr_count = 1;
	trans.data_buf = (UINT32)card_id;
	trans.data_dir = FIFO_DIR_READ;
	trans.data_len = 4;

	return nand_host_issue_transfer(info, &trans);
}

/*
    Read ECC corrected bits

    @return correct bits
*/

int nand_cmd_read_flash_ecc_corrected(struct drv_nand_dev_info *info)
{
#if 0
    	struct smc_setup_trans transParam;
	uint8_t  ecc_bits[4];

	transParam.colAddr = 0;
	transParam.rowAddr = 0;
	transParam.fifoDir = _FIFO_READ;
	transParam.transMode = _PIO_MODE;
	transParam.type = _READ_ID;
	transParam.uiCS	= 0;

	nand_host_setup_transfer(info, &transParam, 0, 4, _SINGLE_PAGE);

	nand_host_send_command(info, SPINAND_CMD_MXIC_READ_ECC, FALSE);

	nand_host_receive_data(info, &ecc_bits[0], 4, _PIO_MODE);

	return ecc_bits[0] & 0xF;
#else
	return 0;
#endif
}

#if 0
/*
	Wait SPI flash ready

	Wait SPI flash returned to ready state

	@param[in] uiWaitMs     Timeout setting. (Unit: ms)

	@return
		- @b E_OK: success
		- @b Else: fail. Maybe timeout.
*/
static ER spiflash_wait_ready(struct drv_nand_dev_info *info, u8 * ucStatus)
{
	union T_NAND_COMMAND_REG uiCmdCycle= {0};
	struct smc_setup_trans transParam;
	ER ret = E_OK;

	do {
		transParam.colAddr = 0;
		transParam.rowAddr = 0;
		transParam.fifoDir = _FIFO_READ;
		transParam.transMode = _PIO_MODE;
		transParam.type = _AUTO_STATUS;
		transParam.uiCS = 0;

		nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);

		uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_RDSR;

		nand_hostSetupStatusCheckBit(info, NAND_STS_BIT_0, NAND_STS_BIT_0_EQUAL_0);

		nand_host_send_command(info, uiCmdCycle.Reg, TRUE);

		ret = nand_cmd_wait_complete(info);

	} while (0);

	if(ucStatus)
		* ucStatus = (UINT8)NAND_GETREG(info, NAND_CTRL_STS_REG_OFS);

	return ret;
}

static ER spiflash_disable_write_latch(struct drv_nand_dev_info *info)
{
	struct smc_setup_trans transParam;
	int  ret;

	transParam.colAddr = 0;
	transParam.rowAddr = 0;
	transParam.fifoDir = _FIFO_READ;
	transParam.transMode = _PIO_MODE;
	transParam.type = _CMD;
	transParam.uiCS = 0;

	nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);

	nand_host_send_command(info, FLASH_CMD_WRDI, FALSE);

	ret = nand_cmd_wait_complete(info);

	return ret;
}

static ER spiflash_enable_write_latch(struct drv_nand_dev_info *info)
{
	struct smc_setup_trans transParam;
	int ret;

	do {
		transParam.colAddr = 0;
		transParam.rowAddr = 0;
		transParam.fifoDir = _FIFO_READ;
		transParam.transMode = _PIO_MODE;
		transParam.type = _CMD;
		transParam.uiCS = 0;

		nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);

		nand_host_send_command(info, FLASH_CMD_WREN, FALSE);

		ret = nand_cmd_wait_complete(info);

		if (ret != E_OK)
			break;
	} while(0);

	return ret;
}
#endif
/*
	Read pages.

	@param[out] buffer Buffer address
	@param pageAddress The address of the page. Only (n * g_pNandInfo->uiBytesPerPageData) will be valid.
			Beware that the max accessible size is 4GB.  One should use nand_readOperation2() instead.
	@param numPage How many pages
	@return
		- @b E_OK       read operation success
		- @b E_PAR      parameter error
		- @b E_SYS
*/
int nand_cmd_read_operation(struct drv_nand_dev_info *info, int8_t * buffer, uint32_t pageAddress, uint32_t numPage)
{
	const struct nvt_nand_flash *f = info->flash_info;

	return spiNand_readPage(info, pageAddress / f->page_size, 0, (uint8_t *)buffer, numPage*f->page_size);
}


/*
	Write pages.(single page operation)

	@param buffer      Buffer address
	@param pageAddress The address of the page. Only (n * g_pNandInfo->uiBytesPerPageData) will be valid.
		Beware that the max accessible size is 4GB.  One should use write_readOperation2() instead.
	@param numPage     How many pages
	@return E_OK or E_SYS
*/
int nand_cmd_write_operation_single(struct drv_nand_dev_info *info, uint32_t pageAddress, uint32_t column)
{
	const struct nvt_nand_flash *f = info->flash_info;
	uint32_t buf_len = info->buf_count - column;

	return spiNand_programPage(info, pageAddress / f->page_size, column, buf_len, 0, 0);
}

/*
	Read the spare data from a page for debug.

	@param[out] spare0  The value of Spare Area Read Data Register 0
	@param[out] spare1  The value of Spare Area Read Data Register 1
	@param[out] spare2  The value of Spare Area Read Data Register 2
	@param[out] spare3  The value of Spare Area Read Data Register 3
	@param pageAddress The address of the page
	@return
		- @b E_OK       read spare success
		- @b E_SYS      read spare operation fail(status fail)
		- @b E_CTX      read spare encounter ecc uncorrect error(Only if Reed solomon ecc usage)
*/
int nand_cmd_read_page_spare_data(struct drv_nand_dev_info *info, int8_t *buffer, uint32_t page_address)
{
	const struct nvt_nand_flash *f = info->flash_info;

	return spiNand_readPage(info, page_address / f->page_size, f->page_size, (uint8_t *)buffer, f->oob_size);
}

/*
    Read the largest number of ecc corrected bits
*/
int nand_cmd_read_ecc_corrected(struct drv_nand_dev_info *info)
{
#if 0
	union T_NAND_RSERR_STS0_REG reg_correct = {0};
	u8 correct_count = 0;

	reg_correct.Reg = NAND_GETREG(info, NAND_RSERR_STS0_REG_OFS);

	/*Read the largest number of correct bits*/
	correct_count = reg_correct.Bit.SEC0_ERR_STS;
	if (reg_correct.Bit.SEC1_ERR_STS > correct_count)
		correct_count = reg_correct.Bit.SEC1_ERR_STS;

	if (reg_correct.Bit.SEC2_ERR_STS > correct_count)
		correct_count = reg_correct.Bit.SEC2_ERR_STS;

	if (reg_correct.Bit.SEC3_ERR_STS > correct_count)
		correct_count = reg_correct.Bit.SEC3_ERR_STS;

	return correct_count;
#else
	return 0;
#endif
}

/*
	Dummy read command

	Dummy read command

	@return
		- @b E_SYS      Status fail
		- @b E_TMOUT    Controller timeout
		- @b E_OK       Operation success

	@note for nand_cmd_wait_complete()
*/
int nand_cmd_dummy_read(struct drv_nand_dev_info *info)
{
	return 0;
}
#if 0
/*
	Read SPI flash

	@param[in] addr	Byte address of SPI flash
	@param[in] byte_size	Byte count of read data. Should be word alignment (i.e. multiple of 4).
	@param[out] pbuf	Pointer point to store read data

	@return void
*/
static int spiflash_read_data(struct drv_nand_dev_info *info, u32 addr, u32 byte_size, u8 *pbuf)
{
	union T_NAND_COMMAND_REG uiCmdCycle= {0};
	struct smc_setup_trans transParam;
	int ret = E_OK;
#if 0
	u32 dummy_read = 0;
#endif
	/*Disable clk_auto_gating before set cs_manual*/
	*(u32*) 0xF00200B0 &= ~0x80000;

	nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);

	nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_MANUAL_MODE);

	nand_hostSetCSActive(info, NAND_SPI_CS_LOW);

	if (info->flash_info->nor_read_mode == SPI_NOR_QUAD_READ) {
		if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_QUAD_READ_NORMAL_4BYTE;
		else
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_QUAD_READ_NORMAL;
	} else if (info->flash_info->nor_read_mode == SPI_NOR_DUAL_READ) {
		if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_DUAL_READ_4BYTE;
		else
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_DUAL_READ;
	} else {
		if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_READ_4BYTE;
		else
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_READ;
	}



	transParam.colAddr      = 0;
	transParam.rowAddr      = addr;
	transParam.fifoDir      = _FIFO_READ;
	transParam.transMode    = _PIO_MODE;
	transParam.type         = _CMD_ADDR;
	transParam.uiCS         = 0;

	nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);

	if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
		nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_4_CYCLE_CNT, NAND_COL_ADDRESS_1_CYCLE_CNT);
	else
		nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_3_CYCLE_CNT, NAND_COL_ADDRESS_1_CYCLE_CNT);

	nand_host_send_command(info, uiCmdCycle.Reg, FALSE);

	ret = nand_cmd_wait_complete(info);

	if(ret != E_OK) {
		nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);
		nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_AUTO_MODE);
		return ret;
	}

	transParam.colAddr      = 0;
	transParam.rowAddr      = 0;
	transParam.fifoDir      = _FIFO_READ;
	transParam.type         = _SPI_READ_N_BYTES;
	transParam.uiCS         = 0;

	if (info->flash_info->nor_read_mode != SPI_NOR_NORMAL_READ) {
		transParam.transMode = _PIO_MODE;
#if 0
		/*Dummy 8 T*/
		nand_host_setup_transfer(info, &transParam, 0, 1, _SINGLE_PAGE);

		nand_host_send_command(info, uiCmdCycle.Reg, 0);

		ret = nand_host_receive_data(info, &dummy_read, 1, _PIO_MODE);
#else
		transParam.type = _DUMMY_CLOCK;
		nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);
		NAND_SETREG(info, NAND_DUMMY_CLOCK_NUM_OFS, 0x7);
		nand_host_send_command(info, uiCmdCycle.Reg, FALSE);
		ret = nand_cmd_wait_complete(info);
#endif
		if(ret != E_OK) {
			nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);
			nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_AUTO_MODE);
			return ret;
		}

		/*Dummy 2 T*/
		if (((info->rdcr_status & 0xC0) == 0xC0) && (info->ops_freq > 96000000)) {
			debug("SF: dummy 10T mode with RDCR 0x%08x\n", info->rdcr_status);
			transParam.type = _DUMMY_CLOCK;
			nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);
			NAND_SETREG(info, NAND_DUMMY_CLOCK_NUM_OFS, 0x1);
			nand_host_send_command(info, uiCmdCycle.Reg, FALSE);
			ret = nand_cmd_wait_complete(info);
			if(ret != E_OK) {
				nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);
				nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_AUTO_MODE);
				return ret;
			}
		}
	}

	transParam.type         = _SPI_READ_N_BYTES;
	transParam.transMode    = _DMA_MODE;

	NAND_SETREG(info, NAND_DMASTART_REG_OFS, dma_getPhyAddr((UINT32)pbuf));

	if (info->flash_info->nor_read_mode == SPI_NOR_QUAD_READ)
		nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_QUAD_MODE, NAND_SPI_ORDER_MODE_1);
	else if (info->flash_info->nor_read_mode == SPI_NOR_DUAL_READ)
		nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_DUAL_MODE, NAND_SPI_ORDER_MODE_1);
	else
		nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_NORMAL_DUPLEX_MODE, NAND_SPI_ORDER_MODE_1);

	nand_host_setup_transfer(info, &transParam, 0, byte_size, _SINGLE_PAGE);

	nand_host_send_command(info, uiCmdCycle.Reg, FALSE);

	ret = nand_cmd_wait_complete(info);

	nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);

	nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_AUTO_MODE);

	if (info->flash_info->nor_read_mode != SPI_NOR_NORMAL_READ)
		nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_NORMAL_DUPLEX_MODE, NAND_SPI_ORDER_MODE_1);

	/*Enable clk_auto_gating after set cs_manual*/
	*(u32*) 0xF00200B0 |= 0x80000;

	return ret;
}

/*
	Page program

	Program a page of SPI flash

	@param[in] addr        Byte address of SPI flash
	@param[in] byte_size       Byte count of written data. Should be <= 256 and word alignment (i.e. multiple of 4).
	@param[out] pbuf            Pointer point to store read data

	@return
		- @b E_OK: program success
		- @b FALSE: program fail. Maybe timeout.
*/
static int spiflash_page_program(struct drv_nand_dev_info *info, u32 addr, u32 byte_size, u8* pbuf)
{
	union T_NAND_COMMAND_REG uiCmdCycle= {0};
	struct smc_setup_trans transParam;
	int ret;
	u8 ucSts;

	spiflash_enable_write_latch(info);

	/*Disable clk_auto_gating before set cs_manual*/
	*(u32*) 0xF00200B0 &= ~0x80000;

	nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);
	nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_MANUAL_MODE);

	nand_hostSetCSActive(info, NAND_SPI_CS_LOW);

	if (info->flash_info->nor_quad_support) {
		if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_PAGEPROG_4X_4BYTE;
		else
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_PAGEPROG_4X;
	} else {
		if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_PAGEPROG_4BYTE;
		else
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_PAGEPROG;
	}

	transParam.colAddr = 0;
	transParam.rowAddr = addr;
	transParam.fifoDir = _FIFO_READ;
	transParam.transMode = _PIO_MODE;
	transParam.type = _CMD_ADDR;
	transParam.uiCS = 0;

	nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);

	if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
		nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_4_CYCLE_CNT, NAND_NOT_CONFIGED_ADDRESS_CYCLE_CNT);
	else
		nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_3_CYCLE_CNT, NAND_NOT_CONFIGED_ADDRESS_CYCLE_CNT);

	nand_host_send_command(info, uiCmdCycle.Reg, FALSE);

	ret = nand_cmd_wait_complete(info);

	if(ret != E_OK) {
		nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);
		nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_AUTO_MODE);
		return ret;
	}

	transParam.fifoDir = _FIFO_WRITE;
	if (byte_size < SPIFLASH_MAX_PROGRAM_BYTE_AT_ONCE)
		transParam.transMode = _PIO_MODE;
	else {
		transParam.transMode = _DMA_MODE;
		NAND_SETREG(info, NAND_DMASTART_REG_OFS, dma_getPhyAddr((UINT32)pbuf));
	}

	transParam.type = _SPI_WRITE_N_BYTES;

	if (info->flash_info->nor_quad_support)
		nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_QUAD_MODE, NAND_SPI_ORDER_MODE_1);
	else
		nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_NORMAL_DUPLEX_MODE, NAND_SPI_ORDER_MODE_1);

	nand_host_setup_transfer(info, &transParam, 0, byte_size, _SINGLE_PAGE);

	nand_host_send_command(info, uiCmdCycle.Reg, FALSE);

	if (byte_size < SPIFLASH_MAX_PROGRAM_BYTE_AT_ONCE)
		ret = nand_host_transmit_data(info, (u8*)pbuf, byte_size, _PIO_MODE);
	else
		ret = nand_cmd_wait_complete(info);

	nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);
	nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_AUTO_MODE);

	/*Enable clk_auto_gating after set cs_manual*/
	*(u32*) 0xF00200B0 |= 0x80000;

	nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_NORMAL_DUPLEX_MODE, NAND_SPI_ORDER_MODE_1);

	if(ret != E_OK)
		return ret;

	ret = spiflash_wait_ready(info, &ucSts);

	spiflash_disable_write_latch(info);

	return ret;
}

int spinor_erase_sector(struct drv_nand_dev_info *info, struct spi_flash *flash, u32 byte_addr)
{
	union T_NAND_COMMAND_REG cmd_cycle = {0};
	struct smc_setup_trans transParam;
	u8 status;
	int ret;

	spiflash_wait_ready(info, &status);

	spiflash_enable_write_latch(info);

	//row addr = 0x44332211 --> 0x44 --> 0x33 --> 0x22 --> 0x11
	//                                            ___     ___________
	//use command + 1 cycle row address cycle ___|cmd|___|Row1stcycle|___

	//use block erase cycle
	//   ____    ______    ______    ______
	//___|cmd|___|addr1|___|addr2|___|addr3|___| ... |___|cmd2|___ ...
	//                                                            ____
	//                                                            |sts|__ ...

	cmd_cycle.Bit.CMD_CYC_1ST = flash->erase_cmd;
	cmd_cycle.Bit.CMD_CYC_2ND = FLASH_CMD_RDSR;
	transParam.colAddr = 0;
	transParam.rowAddr = byte_addr;
	transParam.fifoDir = _FIFO_WRITE;
	transParam.transMode = _PIO_MODE;
	transParam.type = _BLK_ERSE;
	transParam.uiCS = 0;

	if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
		nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_4_CYCLE_CNT, NAND_NOT_CONFIGED_COL_ADDRESS_CYCLE_CNT);
	else
		nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_3_CYCLE_CNT, NAND_NOT_CONFIGED_COL_ADDRESS_CYCLE_CNT);

	nand_hostSetupStatusCheckBit(info, NAND_STS_BIT_0, NAND_STS_BIT_0_EQUAL_0);

	nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);

	nand_host_send_command(info, cmd_cycle.Reg, TRUE);

	ret = nand_cmd_wait_complete(info);

	spiflash_disable_write_latch(info);

	return ret;
}

static int spinor_pio_operation(struct drv_nand_dev_info *info, u32 byte_addr, u32 byte_size, u8 *pbuf)
{
	struct smc_setup_trans transParam;
	union T_NAND_COMMAND_REG uiCmdCycle= {0};
	int ret;

	/*Disable clk_auto_gating before set cs_manual*/
	*(u32*) 0xF00200B0 &= ~0x80000;

	nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);

	nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_MANUAL_MODE);

	nand_hostSetCSActive(info, NAND_SPI_CS_LOW);

	if (info->flash_info->nor_read_mode == SPI_NOR_QUAD_READ) {
		if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_QUAD_READ_NORMAL_4BYTE;
		else
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_QUAD_READ_NORMAL;
	} else if (info->flash_info->nor_read_mode == SPI_NOR_DUAL_READ) {
		if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_DUAL_READ_4BYTE;
		else
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_DUAL_READ;
	} else {
		if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_READ_4BYTE;
		else
			uiCmdCycle.Bit.CMD_CYC_1ST = FLASH_CMD_READ;
	}

	transParam.colAddr      = 0;
	transParam.rowAddr      = byte_addr;
	transParam.fifoDir      = _FIFO_READ;
	transParam.transMode    = _PIO_MODE;
	transParam.type         = _CMD_ADDR;
	transParam.uiCS         = 0;

	nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);

	if (info->flash_info->device_size > SPI_FLASH_16MB_BOUN)
		nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_4_CYCLE_CNT, NAND_COL_ADDRESS_1_CYCLE_CNT);
	else
		nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_3_CYCLE_CNT, NAND_COL_ADDRESS_1_CYCLE_CNT);

	nand_host_send_command(info, uiCmdCycle.Reg, FALSE);

	ret = nand_cmd_wait_complete(info);

	if (ret != E_OK) {
		nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);
		nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_AUTO_MODE);
		return ret;
	}

	if (info->flash_info->nor_read_mode != SPI_NOR_NORMAL_READ) {
		transParam.colAddr    = 0;
		transParam.rowAddr    = 0;
		transParam.transMode  = _PIO_MODE;
		transParam.type       = _DUMMY_CLOCK;

		nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);

		NAND_SETREG(info, NAND_DUMMY_CLOCK_NUM_OFS, 0x7);

		nand_host_send_command(info, uiCmdCycle.Reg, FALSE);

		ret = nand_cmd_wait_complete(info);

		if(ret != E_OK) {
			nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);
			nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_AUTO_MODE);
			return ret;
		}

		//Dummy 2 T
		if (((info->rdcr_status & 0xC0) == 0xC0) && (info->ops_freq > 96000000)) {
			debug("SF: dummy 10T mode with RDCR 0x%08x\n", info->rdcr_status);

			transParam.type = _DUMMY_CLOCK;

			nand_host_setup_transfer(info, &transParam, 0, 0, _SINGLE_PAGE);

			NAND_SETREG(info, NAND_DUMMY_CLOCK_NUM_OFS, 0x1);

			nand_host_send_command(info, uiCmdCycle.Reg, FALSE);

			ret = nand_cmd_wait_complete(info);

			if(ret != E_OK) {
				nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);
				nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_AUTO_MODE);
				return ret;
			}
		}
	}

	transParam.colAddr = 0;
	transParam.rowAddr = 0;
	transParam.fifoDir = _FIFO_READ;
	transParam.type = _SPI_READ_N_BYTES;

	if (info->flash_info->nor_read_mode == SPI_NOR_QUAD_READ)
		nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_QUAD_MODE, NAND_SPI_ORDER_MODE_1);
	else if (info->flash_info->nor_read_mode == SPI_NOR_DUAL_READ)
		nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_DUAL_MODE, NAND_SPI_ORDER_MODE_1);
	else
		nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_NORMAL_DUPLEX_MODE, NAND_SPI_ORDER_MODE_1);

	nand_host_setup_transfer(info, &transParam, 0, byte_size, _SINGLE_PAGE);

	nand_host_send_command(info, uiCmdCycle.Reg, FALSE);

	ret = nand_host_receive_data(info, pbuf, byte_size, _PIO_MODE);

	nand_hostSetCSActive(info, NAND_SPI_CS_HIGH);

	nand_hostConfigChipSelOperationMode(info, NAND_SPI_CS_AUTO_MODE);

	if (info->flash_info->nor_read_mode != SPI_NOR_NORMAL_READ)
		nand_hostSetSPIIORegister(info, NAND_SPI_CS_ACTIVE_LOW, NAND_SPI_NORMAL_DUPLEX_MODE, NAND_SPI_ORDER_MODE_1);

	/*Enable clk_auto_gating after set cs_manual*/
	*(u32*) 0xF00200B0 |= 0x80000;

	return ret;
}


/*
	Read SPI flash data

	Read data from SPI flash.

	@param[in] byte_addr   Byte address of flash.
	@param[in] byte_size   Byte size of read data.
	@param[out] pbuf        Pointer point to store read data

	@return
		- @b E_OK: erase success
		- @b E_CTX: driver not opened
		- @b Else: read fail
*/
int spinor_read_operation(struct drv_nand_dev_info *info, u32 byte_addr, u32 byte_size, u8 *pbuf)
{
	u32 remain_byte, read_cycle, addr_index, cycle_index;
	u32 align_length, remain_length;
	int ret = E_SYS;

	/*Handle if dram starting address is not cacheline alignment*/
	if ((u32)pbuf & (CONFIG_SYS_CACHELINE_SIZE-1)) {
		align_length = (u32)pbuf & (CONFIG_SYS_CACHELINE_SIZE-1);
		align_length = CONFIG_SYS_CACHELINE_SIZE - align_length;

		ret = spinor_pio_operation(info, byte_addr , align_length, pbuf);
		if (ret != E_OK) {
			printf("%s: fail at flash address 0x%x length 0x%x\r\n", __func__, byte_addr, align_length);
			return ret;
		}

		if (byte_size <= align_length)
			return E_OK;
		else
			byte_size -= align_length;

		byte_addr += align_length;

		pbuf += align_length;
	}

	align_length = (byte_size / CONFIG_SYS_CACHELINE_SIZE) * \
			CONFIG_SYS_CACHELINE_SIZE;

	remain_length = ((byte_size - align_length + 3) / 4) * 4;

	read_cycle = align_length / SPIFLASH_MAX_READ_BYTE_AT_ONCE;

	addr_index = byte_addr;

	remain_byte = align_length % SPIFLASH_MAX_READ_BYTE_AT_ONCE;

	if (align_length) {
		for(cycle_index = 0; cycle_index < read_cycle; cycle_index++, addr_index += SPIFLASH_MAX_READ_BYTE_AT_ONCE) {
			ret = spiflash_read_data(info, addr_index, SPIFLASH_MAX_READ_BYTE_AT_ONCE, (pbuf + (cycle_index * SPIFLASH_MAX_READ_BYTE_AT_ONCE)));
			if (ret != E_OK) {
				printf("%s: fail at flash address 0x%x\r\n", __func__, addr_index);
				break;
			} else
				debug("R");
		}

		if(remain_byte) {
			ret = spiflash_read_data(info, addr_index, remain_byte, (pbuf + (cycle_index * SPIFLASH_MAX_READ_BYTE_AT_ONCE)));
			if (ret != E_OK)
				printf("%s: fail at flash address 0x%x\r\n", __func__, addr_index);
			else
				debug("r");
		}
		invalidate_dcache_range((u32)pbuf, (u32)(pbuf + align_length));
	}

	/*Handle if length is not cacheline alignment*/
	if (remain_length) {
		ret = spinor_pio_operation(info, byte_addr + align_length , remain_length, pbuf + align_length);
		if (ret != E_OK)
			printf("%s: fail at flash address 0x%x length 0x%x\r\n", __func__, byte_addr + align_length, remain_length);
	}

	return ret;
}

/*
	Program a sector

	Program a sector of SPI flash.

	@param[in] byte_addr   Byte address of programed sector. Should be sector size alignment.
	@param[in] sector_size Byte size of one secotr. Should be sector size.
	@param[in] pbuf         pointer point to written data

	@return
		- @b E_OK: erase success
		- @b E_CTX: driver not opened
		- @b E_MACV: byte_addr is set into s/w write protect detect region
		- @b E_TMOUT: flash ready timeout
		- @b E_PAR: byte_addr is not sector size alignment
		- @b Else: program fail
*/
int spinor_program_operation(struct drv_nand_dev_info *info, u32 byte_addr, u32 sector_size, u8* pbuf)
{
	u32 program_cycle;
	u32 addr_index;
	u32 cycle_index;
	int ret = E_SYS;

	program_cycle = sector_size / SPIFLASH_MAX_PROGRAM_BYTE_AT_ONCE;

	addr_index = byte_addr;

	flush_dcache_range((u32)pbuf, (u32)(pbuf + sector_size));

	for(cycle_index = 0; cycle_index < program_cycle; cycle_index++, addr_index += SPIFLASH_MAX_PROGRAM_BYTE_AT_ONCE) {
		ret = spiflash_page_program(info, addr_index, SPIFLASH_MAX_PROGRAM_BYTE_AT_ONCE, (pbuf + (cycle_index * SPIFLASH_MAX_PROGRAM_BYTE_AT_ONCE)));

		if (ret != E_OK) {
			printf("%s: fail at flash address 0x%x\r\n", __func__, addr_index);
			break;
		} else
			debug("W");

		sector_size -= SPIFLASH_MAX_PROGRAM_BYTE_AT_ONCE;
	}

	if (sector_size) {

		/*Minimal transmitting size should be word align*/
		if (sector_size % WORD_ALIGN_OFFSET)
			sector_size += WORD_ALIGN_OFFSET - (sector_size % WORD_ALIGN_OFFSET);

		ret = spiflash_page_program(info, addr_index, sector_size, (pbuf + (cycle_index * SPIFLASH_MAX_PROGRAM_BYTE_AT_ONCE)));
	}

	return ret;
}

int spinor_read_status(struct drv_nand_dev_info *info, u8 status_set, u8* status)
{
	struct smc_setup_trans trans_param;
	u32 spi_cmd = 0;
	int ret = E_OK;

	do {
		trans_param.colAddr = 0;
		trans_param.rowAddr = 0;
		trans_param.fifoDir = _FIFO_READ;
		trans_param.transMode = _PIO_MODE;
		trans_param.type = _CMD_STS;
		trans_param.uiCS = 0;

		nand_host_setup_transfer(info, &trans_param, 0, 0, _SINGLE_PAGE);

		if (status_set == NAND_SPI_NOR_STS_RDSR_1)
			spi_cmd = FLASH_CMD_RDSR;
		else if(status_set == NAND_SPI_NOR_STS_RDCR)
			spi_cmd = FLASH_CMD_RDCR;
		else
			spi_cmd = FLASH_CMD_RDSR2;

		nand_host_send_command(info, spi_cmd, FALSE);

		ret = nand_cmd_wait_complete(info);

	} while (0);

	*status = NAND_GETREG(info, NAND_CTRL_STS_REG_OFS);

	return ret;
}

int spinor_write_status(struct drv_nand_dev_info *info, u8 status_set, u8 status)
{
	union T_NAND_COMMAND_REG cmd_cycle = {0};
	struct smc_setup_trans trans_param;
	u32 spi_cmd = 0;
	int ret = E_OK;
	u8 wait_status = 0;

	spiflash_enable_write_latch(info);

	do {
		//row addr = 0x44332211 --> 0x44 --> 0x33 --> 0x22 --> 0x11
		//                                            ___     ___________
		//use command + 1 cycle row address cycle ___|cmd|___|Row1stcycle|___

		if (status_set == NAND_SPI_NOR_STS_WRSR_1)
			cmd_cycle.Bit.CMD_CYC_1ST = FLASH_CMD_WRSR;
		else
			cmd_cycle.Bit.CMD_CYC_1ST = FLASH_CMD_WRSR2;

		spi_cmd = status;

		trans_param.colAddr = 0;
		trans_param.rowAddr = spi_cmd;
		trans_param.fifoDir = _FIFO_READ;
		trans_param.transMode = _PIO_MODE;
		trans_param.type = _CMD_ADDR;
		trans_param.uiCS = 0;

		nand_host_setup_transfer(info, &trans_param, 0, 0, _SINGLE_PAGE);

		if (info->flash_info->qe_flags & SPI_FLASH_BUSWIDTH_QUAD_TYPE4)
			nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_2_CYCLE_CNT, NAND_COL_ADDRESS_1_CYCLE_CNT);
		else
			nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_1_CYCLE_CNT, NAND_COL_ADDRESS_1_CYCLE_CNT);

		nand_host_send_command(info, cmd_cycle.Reg, FALSE);

		ret = nand_cmd_wait_complete(info);

		if (ret)
			return ret;

	} while (0);

	ret = spiflash_wait_ready(info, &wait_status);
	if (ret)
		printf("wait ready timeout with status 0x%x\n", wait_status);

	return ret;
}

int spi_nor_set_status_half_word(struct drv_nand_dev_info *info, u16 hstatus)
{
	union T_NAND_COMMAND_REG cmd_cycle = {0};
	struct smc_setup_trans trans_param;
	u32 spi_cmd = 0;
	int ret = E_OK;
	u8 wait_status = 0;

	spiflash_enable_write_latch(info);

	do {
		cmd_cycle.Bit.CMD_CYC_1ST = FLASH_CMD_WRSR;

		spi_cmd = hstatus;

		trans_param.colAddr = 0;
		trans_param.rowAddr = spi_cmd;
		trans_param.fifoDir = _FIFO_READ;
		trans_param.transMode = _PIO_MODE;
		trans_param.type = _CMD_ADDR;
		trans_param.uiCS = 0;

		nand_host_setup_transfer(info, &trans_param, 0, 0, _SINGLE_PAGE);

		/*Note CMD address is msb first*/
		/*Ex 0x00000740 will send __|07|__|40|__*/
		nand_hostSetupAddressCycle(0, NAND_ROW_ADDRESS_2_CYCLE_CNT, NAND_COL_ADDRESS_1_CYCLE_CNT);

		nand_host_send_command(info, cmd_cycle.Reg, FALSE);

		ret = nand_cmd_wait_complete(info);

		if (ret)
			return ret;
	} while (0);

	ret = spiflash_wait_ready(info, &wait_status);
	if (ret)
		printf("wait ready timeout with status 0x%x\n", wait_status);

	return ret;
}
#endif

void flash_copy_info(struct drv_nand_dev_info *info)
{
	drvdump_info = *info;
}