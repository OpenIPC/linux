/*
 * The Flash Memory Controller v100 Device Driver for xmedia
 *
 * Copyright (c) 2016 XMEDIA Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
    Send set/get features command to SPI Nand flash
*/
char spi_nand_feature_op(struct xmedia_fmc_spi *spi, u_char op, u_char addr,
			   u_char *val)
{
	unsigned int reg;
	const char *str[] = {"Get", "Set"};
	struct xmedia_fmc_host *host = NULL;

	if (!spi) {
		DB_MSG("Error: spi is NULL !\n");
		return -1;
	}
	host = (struct xmedia_fmc_host *)spi->host;
	if (!host) {
		DB_MSG("Error: host is NULL !\n");
		return -1;
	}

	if ((op == GET_OP) && (STATUS_ADDR == addr)) {
		if (!val) {
			DB_MSG("Error: val is NULL !\n");
			return -1;
		}
		if (SR_DBG) {
			pr_info("\n");
		}
		FMC_PR(SR_DBG, "\t\t|*-Start Get Status\n");

		reg = OP_CFG_FM_CS(host->cmd_op.cs) | OP_CFG_OEN_EN;
		fmc_writel(host, FMC_OP_CFG, reg);
		FMC_PR(SR_DBG, "\t\t||-Set OP_CFG[%#x]%#x\n", FMC_OP_CFG, reg);

		reg = FMC_OP_READ_STATUS_EN | FMC_OP_REG_OP_START;
		fmc_writel(host, FMC_OP, reg);
		FMC_PR(SR_DBG, "\t\t||-Set OP[%#x]%#x\n", FMC_OP, reg);

		FMC_CMD_WAIT_CPU_FINISH(host);

		*val = fmc_readl(host, FMC_STATUS);
		FMC_PR(SR_DBG, "\t\t|*-End Get Status, result: %#x\n", *val);

		return 0;
	}

	FMC_PR(FT_DBG, "\t|||*-Start %s feature, addr[%#x]\n", str[op], addr);

	xmedia_fmc100_ecc0_switch(host, ENABLE);

	reg = FMC_CMD_CMD1(op ? SPI_CMD_SET_FEATURE : SPI_CMD_GET_FEATURES);
	fmc_writel(host, FMC_CMD, reg);
	FMC_PR(FT_DBG, "\t||||-Set CMD[%#x]%#x\n", FMC_CMD, reg);

	fmc_writel(host, FMC_ADDRL, addr);
	FMC_PR(FT_DBG, "\t||||-Set ADDRL[%#x]%#x\n", FMC_ADDRL, addr);

	reg = OP_CFG_FM_CS(host->cmd_op.cs)
	      | OP_CFG_ADDR_NUM(FEATURES_OP_ADDR_NUM)
	      | OP_CFG_OEN_EN;
	fmc_writel(host, FMC_OP_CFG, reg);
	FMC_PR(FT_DBG, "\t||||-Set OP_CFG[%#x]%#x\n", FMC_OP_CFG, reg);

	reg = FMC_DATA_NUM_CNT(FEATURES_DATA_LEN);
	fmc_writel(host, FMC_DATA_NUM, reg);
	FMC_PR(FT_DBG, "\t||||-Set DATA_NUM[%#x]%#x\n", FMC_DATA_NUM, reg);

	reg = FMC_OP_CMD1_EN
	      | FMC_OP_ADDR_EN
	      | FMC_OP_REG_OP_START;

	if (op == SET_OP) {
		if (!val || !host->iobase) {
			DB_MSG("Error: host->iobase is NULL !\n");
			return -1;
		}
		reg |= FMC_OP_WRITE_DATA_EN;
		fmc_writeb(*val, host->iobase);
		FMC_PR(FT_DBG, "\t||||-Write IO[%#lx]%#x\n", (long)host->iobase,
		       *(u_char *)host->iobase);
	} else {
		reg |= FMC_OP_READ_DATA_EN;
	}

	fmc_writel(host, FMC_OP, reg);
	FMC_PR(FT_DBG, "\t||||-Set OP[%#x]%#x\n", FMC_OP, reg);

	FMC_CMD_WAIT_CPU_FINISH(host);

	if (op == GET_OP) {
		if (!val || !host->iobase) {
			DB_MSG("Error: val or host->iobase is NULL !\n");
			return -1;
		}
		*val = fmc_readb(host->iobase);
		FMC_PR(FT_DBG, "\t||||-Read IO[%#lx]%#x\n", (long)host->iobase,
		       *(u_char *)host->iobase);
	}

	xmedia_fmc100_ecc0_switch(host, DISABLE);

	FMC_PR(FT_DBG, "\t|||*-End %s Feature[%#x]:%#x\n", str[op], addr, *val);

	return 0;
}

/*
    Read status[C0H]:[0]bit OIP, judge whether the device is busy or not
*/
static int spi_general_wait_ready(struct xmedia_fmc_spi *spi)
{
	unsigned char status;
	int ret;
	unsigned long deadline = jiffies + FMC_MAX_READY_WAIT_JIFFIES;
	struct xmedia_fmc_host *host = NULL;

	if(spi == NULL || spi->host == NULL) {
		DB_MSG("Error: host or host->spi is NULL!\n");
		return -1;
	}
	host = (struct xmedia_fmc_host *)spi->host;

	do {
		ret = spi_nand_feature_op(spi, GET_OP, STATUS_ADDR, &status);
		if (ret)
		    return -1;
		if (!(status & STATUS_OIP_MASK)) {
			if ((host->cmd_op.l_cmd == NAND_CMD_ERASE2)
					&& (status & STATUS_E_FAIL_MASK)) {
				return status;
			}
			if ((host->cmd_op.l_cmd == NAND_CMD_PAGEPROG)
					&& (status & STATUS_P_FAIL_MASK)) {
				return status;
			}
			return 0;
		}

		cond_resched();
	} while (!time_after_eq(jiffies, deadline));

	DB_MSG("Error: SPI Nand wait ready timeout, status: %#x\n", status);

	return 1;
}

/*
    Send write enable cmd to SPI Nand, status[C0H]:[2]bit WEL must be set 1
*/
static int spi_general_write_enable(struct xmedia_fmc_spi *spi)
{
	u_char reg;
	int ret;
	unsigned int regl;
	struct xmedia_fmc_host *host = NULL;
	if(spi == NULL || spi->host == NULL) {
		DB_MSG("Error: host or host->spi is NULL!\n");
		return -1;
	}
	host = spi->host;
	if (WE_DBG) {
		pr_info("\n");
	}
	FMC_PR(WE_DBG, "\t|*-Start Write Enable\n");

	ret = spi_nand_feature_op(spi, GET_OP, STATUS_ADDR, &reg);
	if (ret)
		return -1;
	if (reg & STATUS_WEL_MASK) {
		FMC_PR(WE_DBG, "\t||-Write Enable was opened! reg: %#x\n",
		       reg);
		return 0;
	}

	regl = fmc_readl(host, FMC_GLOBAL_CFG);
	FMC_PR(WE_DBG, "\t||-Get GLOBAL_CFG[%#x]%#x\n", FMC_GLOBAL_CFG, regl);
	if (regl & FMC_GLOBAL_CFG_WP_ENABLE) {
		regl &= ~FMC_GLOBAL_CFG_WP_ENABLE;
		fmc_writel(host, FMC_GLOBAL_CFG, regl);
		FMC_PR(WE_DBG, "\t||-Set GLOBAL_CFG[%#x]%#x\n",
		       FMC_GLOBAL_CFG, regl);
	}

	regl = FMC_CMD_CMD1(SPI_CMD_WREN);
	fmc_writel(host, FMC_CMD, regl);
	FMC_PR(WE_DBG, "\t||-Set CMD[%#x]%#x\n", FMC_CMD, regl);

	regl = OP_CFG_FM_CS(host->cmd_op.cs) | OP_CFG_OEN_EN;
	fmc_writel(host, FMC_OP_CFG, regl);
	FMC_PR(WE_DBG, "\t||-Set OP_CFG[%#x]%#x\n", FMC_OP_CFG, regl);

	regl = FMC_OP_CMD1_EN | FMC_OP_REG_OP_START;
	fmc_writel(host, FMC_OP, regl);
	FMC_PR(WE_DBG, "\t||-Set OP[%#x]%#x\n", FMC_OP, regl);

	FMC_CMD_WAIT_CPU_FINISH(host);

#if WE_DBG
	if(!spi->driver) {
		DB_MSG("Error: spi->driver is NULL!\n");
		return -1;
	}
	spi->driver->wait_ready(spi);

	ret = spi_nand_feature_op(spi, GET_OP, STATUS_ADDR, &reg);
	if (ret)
		return -1;
	if (reg & STATUS_WEL_MASK) {
		FMC_PR(WE_DBG, "\t||-Write Enable success. reg: %#x\n", reg);
	} else {
		DB_MSG("Error: Write Enable failed! reg: %#x\n", reg);
		return reg;
	}
#endif

	FMC_PR(WE_DBG, "\t|*-End Write Enable\n");
	return 0;
}

/*
    judge whether SPI Nand support QUAD read/write or not
*/
static int spi_is_quad(struct xmedia_fmc_spi *spi)
{
	const char *if_str[] = {"STD", "DUAL", "DIO", "QUAD", "QIO"};
	FMC_PR(QE_DBG, "\t\t|||*-SPI read iftype: %s write iftype: %s\n",
	       if_str[spi->read->iftype], if_str[spi->write->iftype]);

	if ((spi->read->iftype == IF_TYPE_QUAD)
			|| (spi->read->iftype == IF_TYPE_QIO)
			|| (spi->write->iftype == IF_TYPE_QUAD)
			|| (spi->write->iftype == IF_TYPE_QIO)) {
		return 1;
	}

	return 0;
}

/*
    Send set features cmd to SPI Nand, feature[B0H]:[0]bit QE would be set
*/
static int spi_general_qe_enable(struct xmedia_fmc_spi *spi)
{
	int op;
	u_char reg;
	int ret;
	const char *str[] = {"Disable", "Enable"};

	if(!spi || !spi->host || !spi->driver) {
		DB_MSG("Error: host or spi->host or spi->driver is NULL!\n");
		return -1;
	}
	FMC_PR(QE_DBG, "\t||*-Start SPI Nand flash QE\n");

	op = spi_is_quad(spi);

	FMC_PR(QE_DBG, "\t|||*-End Quad check, SPI Nand %s Quad.\n", str[op]);

	ret = spi_nand_feature_op(spi, GET_OP, FEATURE_ADDR, &reg);
	if (ret)
		return -1;
	FMC_PR(QE_DBG, "\t|||-Get [%#x]feature: %#x\n", FEATURE_ADDR, reg);
	if ((reg & FEATURE_QE_ENABLE) == op) {
		FMC_PR(QE_DBG, "\t||*-SPI Nand quad was %sd!\n", str[op]);
		return op;
	}

	if (op == ENABLE) {
		reg |= FEATURE_QE_ENABLE;
	} else {
		reg &= ~FEATURE_QE_ENABLE;
	}

	ret = spi_nand_feature_op(spi, SET_OP, FEATURE_ADDR, &reg);
	if (ret)
		return -1;
	FMC_PR(QE_DBG, "\t|||-SPI Nand %s Quad\n", str[op]);

	spi->driver->wait_ready(spi);

	ret = spi_nand_feature_op(spi, GET_OP, FEATURE_ADDR, &reg);
	if (ret)
		return -1;
	if ((reg & FEATURE_QE_ENABLE) == op) {
		FMC_PR(QE_DBG, "\t|||-SPI Nand %s Quad succeed!\n", str[op]);
	} else {
		DB_MSG("Error: %s Quad failed! reg: %#x\n", str[op], reg);
	}

	FMC_PR(QE_DBG, "\t||*-End SPI Nand %s Quad.\n", str[op]);

	return op;
}
