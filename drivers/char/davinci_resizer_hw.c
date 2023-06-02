/* *
 * Copyright (C) 2006 Texas Instruments Inc
 *
 * This program is free software you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/davinci_resizer.h>
#include "davinci_resizer_hw.h"

/*
 * Function to set hardware configuration registers
 */
void rsz_hardware_setup(struct channel_config *rsz_conf_chan)
{
	/* Counter to set the value of horizonatl and vertical coeff */
	int coeffcounter;
	/* for getting the coefficient offset */
	int coeffoffset = ZERO;

	/* clear the VPSS_PCR register buffer overflow bits     */
	regw(0x003c0000, VPSS_PCR);

	/* setting the hardware register rszcnt */
	regw(rsz_conf_chan->register_config.rsz_cnt, RSZ_CNT);

	dev_dbg(rsz_device, "RSZ CNT : %x regr = %x \n",
		rsz_conf_chan->register_config.rsz_cnt, regr(RSZ_CNT));

	/* setting the hardware register instart */
	regw(rsz_conf_chan->register_config.rsz_in_start, IN_START);

	dev_dbg(rsz_device, "In START %x regr = %x\n",
		rsz_conf_chan->register_config.rsz_in_start, regr(IN_START));

	/* setting the hardware register insize */
	regw(rsz_conf_chan->register_config.rsz_in_size, IN_SIZE);

	dev_dbg(rsz_device, "In size %x regr = %x\n",
		rsz_conf_chan->register_config.rsz_in_size, regr(IN_SIZE));
	/* setting the hardware register outsize */
	regw(rsz_conf_chan->register_config.rsz_out_size, OUT_SIZE);

	dev_dbg(rsz_device, "out size %x regr = %x\n",
		rsz_conf_chan->register_config.rsz_out_size, regr(OUT_SIZE));

	/* setting the hardware register inaddress */
	regw(rsz_conf_chan->register_config.rsz_sdr_inadd, SDR_INADD);

	dev_dbg(rsz_device, "in address %x regr = %x\n",
		rsz_conf_chan->register_config.rsz_sdr_inadd, regr(SDR_INADD));
	/* setting the hardware register in offset */
	regw(rsz_conf_chan->register_config.rsz_sdr_inoff, SDR_INOFF);

	dev_dbg(rsz_device, "in offset %x regr = %x\n",
		rsz_conf_chan->register_config.rsz_sdr_inoff, regr(SDR_INOFF));
	/* setting the hardware register in offset */
	/* setting the hardware register out address */
	regw(rsz_conf_chan->register_config.rsz_sdr_outadd, SDR_OUTADD);

	dev_dbg(rsz_device, "out addrsess %x regr = %x\n",
		rsz_conf_chan->register_config.rsz_sdr_outadd,
		regr(SDR_OUTADD));

	/* setting the hardware register in offset */
	/* setting the hardware register out offset */
	regw(rsz_conf_chan->register_config.rsz_sdr_outoff, SDR_OUTOFF);

	dev_dbg(rsz_device, "out offset %x regr = %x\n",
		rsz_conf_chan->register_config.rsz_sdr_outoff,
		regr(SDR_OUTOFF));

	/* setting the hardware register yehn */
	regw(rsz_conf_chan->register_config.rsz_yehn, YENH);

	dev_dbg(rsz_device, "yehn  %x regr = %x\n",
		rsz_conf_chan->register_config.rsz_yehn, regr(YENH));

	/* setting the hardware registers of coefficients */
	for (coeffcounter = ZERO; coeffcounter < MAX_COEF_COUNTER;
	     coeffcounter++) {
		regw(rsz_conf_chan->register_config.
		     rsz_coeff_horz[coeffcounter], ((HFILT10 + coeffoffset)));

		regw(rsz_conf_chan->register_config.
		     rsz_coeff_vert[coeffcounter], ((VFILT10 + coeffoffset)));
		coeffoffset = coeffoffset + COEFF_ADDRESS_OFFSET;
	}
}

/*
 * Function to enable the resizer
 */
int rsz_enable_dvrz(struct channel_config *rsz_conf_chan)
{
	/* Eanbling the resizer the setting enable bit */
	rsz_conf_chan->register_config.rsz_pcr =
	    BITSET(rsz_conf_chan->register_config.rsz_pcr, SET_ENABLE_BIT);

	regw(rsz_conf_chan->register_config.rsz_pcr, PCR);

	dev_dbg(rsz_device, "the value of pcr is %x \n", regr(PCR));

	regw(0x003c0000, VPSS_PCR);

	return SUCESS;
}

int rsz_writebuffer_status(void)
{
	dev_dbg(rsz_device, "VPSS_PCR: %x\n", regr(VPSS_PCR));
	return (regr(VPSS_PCR) >> 18) & 0xF;
}

