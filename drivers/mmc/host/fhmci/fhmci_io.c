
#include <mach/clock.h>

#define SIMPLE_0
//#define SIMPLE_90
//#define SIMPLE_180
//#define SIMPLE_270

static int fh_mci_sys_ctrl_init(struct fhmci_host *host)
{

	struct clk *sdc_clk;
	u32 pctrl_gpio;
	u32 drv_shift;
	u32 sam_shift;
	int ret;
	u32 reg;
	host->pdata->fifo_depth = (fhmci_readl(host->base + MCI_FIFOTH) & 0xfff0000) >> 16;
	if (host->id == 0) {
		pctrl_gpio = 5;
		drv_shift = 20;
		sam_shift = 16;
		sdc_clk = clk_get(NULL, "sdc0_clk");

	} else {
		pctrl_gpio = 6;
		drv_shift = 12;
		sam_shift = 8;
		sdc_clk = clk_get(NULL, "sdc1_clk");

	}

	//Power on.
	ret = gpio_request(pctrl_gpio, NULL);
	if (ret) {
		fhmci_error("gpio requset err!");
		return ret;
	}
	gpio_direction_output(pctrl_gpio, 0);
	gpio_free(pctrl_gpio);

	//adjust clock phase...
	clk_enable(sdc_clk);
	clk_set_rate(sdc_clk, 50000000);
	reg = clk_get_clk_sel();
	reg &= ~(3 << drv_shift);
	reg &= ~(3 << sam_shift);
	reg |= (2 << drv_shift); //now drv fixed to 180.
#ifdef SIMPLE_0
	reg |= (0 << sam_shift);
#endif
#ifdef SIMPLE_90
	reg |= (1 << sam_shift);
#endif
#ifdef SIMPLE_180
	reg |= (2 << sam_shift);
#endif
#ifdef SIMPLE_270
	reg |= (3 << sam_shift);
#endif
	clk_set_clk_sel(reg);
	return 0;
}
