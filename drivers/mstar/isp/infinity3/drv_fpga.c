#include "drv_fpga.h"
#define BANK_BASE_ADDR (0x1F000000)
unsigned int getTvtoolBankAddrVal(u32 bank, u8 offset) {
	void __iomem *phys_addr;
	phys_addr = (void __iomem *)IO_ADDRESS((BANK_BASE_ADDR + bank*0x200 + offset*0x04));
	return readl(phys_addr);
}

unsigned int setTvtoolBankAddrVal(u32 bank, u8 offset, u16 val) {
	void __iomem *phys_addr;
	phys_addr = (void __iomem *)IO_ADDRESS((BANK_BASE_ADDR + bank*0x200 + (offset>>1)*0x04));
	writel(val, phys_addr);
	return 0;
}

void fpgaInit(void)
{
	// ISP setting
	//SET CCM and GAMMA
	setTvtoolBankAddrVal(0x1302,0x64, 0x0000);
	setTvtoolBankAddrVal(0x1302,0x66, 0x0400);
	setTvtoolBankAddrVal(0x1302,0x68, 0x0000);
	setTvtoolBankAddrVal(0x1302,0x6a, 0x0000);
	setTvtoolBankAddrVal(0x1302,0x6c, 0x0000);
	setTvtoolBankAddrVal(0x1302,0x6e, 0x0400);
	setTvtoolBankAddrVal(0x1302,0x70, 0x0000);
	setTvtoolBankAddrVal(0x1302,0x72, 0x0000);
	setTvtoolBankAddrVal(0x1302,0x74, 0x0000);
	setTvtoolBankAddrVal(0x1302,0x76, 0x0400);
	setTvtoolBankAddrVal(0x1302,0x78, 0x0191); //0x3C: CMC enable
	setTvtoolBankAddrVal(0x1302,0x7a, 0x01c2);
	setTvtoolBankAddrVal(0x1302,0x7c, 0x1e87);
	setTvtoolBankAddrVal(0x1302,0x7e, 0x1fb7);
	setTvtoolBankAddrVal(0x1302,0x80, 0x0107);
	setTvtoolBankAddrVal(0x1302,0x82, 0x0204);
	setTvtoolBankAddrVal(0x1302,0x84, 0x0064);
	setTvtoolBankAddrVal(0x1302,0x86, 0x1f68);
	setTvtoolBankAddrVal(0x1302,0x88, 0x1ed6);
	setTvtoolBankAddrVal(0x1302,0x8a, 0x01c2);
	setTvtoolBankAddrVal(0x1302,0x60, 0x0000);
	setTvtoolBankAddrVal(0x1302,0xe2, 0x0d07);
	setTvtoolBankAddrVal(0x1302,0xe4, 0x2701);
	setTvtoolBankAddrVal(0x1302,0xe6, 0x040d);
	setTvtoolBankAddrVal(0x1302,0xe0, 0x5601); //CFAI enable , DEMOSAIC
	setTvtoolBankAddrVal(0x1302,0x22, 0x0002); //bayer ID
	setTvtoolBankAddrVal(0x1038,0xC2, 0x0001); //0x61, enable isp clk
	setTvtoolBankAddrVal(0x1038,0xC4, 0x010C); //0x62, enable pclk 86MHz
	//swch 4
	setTvtoolBankAddrVal(0x1302,0x02, 0x0000); //0x00, reset ISP
	setTvtoolBankAddrVal(0x1302,0x02, 0x8003); //0x01, disable double buffer
	setTvtoolBankAddrVal(0x1302,0x08, 0x0210); //0x01, isp output async mode = 1,isp_if_rmux = RDMA, isp_wdma_mux=isp_dp
	setTvtoolBankAddrVal(0x1302,0x0A, 0x0000); //0x0A, reg_isp_if_src_sel from sensor
	setTvtoolBankAddrVal(0x1302,0x28, 0x04FF); //0x14, isp crop width 1280
	setTvtoolBankAddrVal(0x1302,0x2A, 0x02CF); //0x15, isp crop height 720
	setTvtoolBankAddrVal(0x1302,0x20, 0x1500); //0x10, sensor formate 10 bits , RGB , reg_sensor_hsync_polarity=low active
	setTvtoolBankAddrVal(0x1302,0x00, 0x0003); //0x00, enable ISP

	setTvtoolBankAddrVal(0x1308,0x40, 0x0291); //0x20, enable wdma , pack mode = 16
	setTvtoolBankAddrVal(0x1302,0x2C, 0x0000); //0x16, wdma start x 0
	setTvtoolBankAddrVal(0x1302,0x2E, 0x0000); //0x17, wdma start y 0
	setTvtoolBankAddrVal(0x1302,0x30, 0x04FF); //0x18, wdma width 1280
	setTvtoolBankAddrVal(0x1302,0x32, 0x02CF); //0x19, wdma height 720
	setTvtoolBankAddrVal(0x1308,0x42, 0x00A0); //0x21, wdma pitch
	setTvtoolBankAddrVal(0x1308,0x50, 0x0000); //0x29, wdma dest low addr
	setTvtoolBankAddrVal(0x1308,0x52, 0x0040); //0x29, wdma dest high addr at 16MB
	setTvtoolBankAddrVal(0x1308,0x60, 0x0100); //0x30, wdma trigger

	/////// ISP SENSOR /////
	//MCLK=12M
	//PCLK=24M
	//VSYNC=31ms,for reset

	//wriu -w 0x101E0a  0x2000   //reg_sr_i2c_mode,bit13,14
	setTvtoolBankAddrVal(0x101E, 0x0c, 0x0014);  // bit [0,1,2] reg_sr_mode,[4,5]reg_i2c_mode
	setTvtoolBankAddrVal(0x101E, 0xa0, 0x0000);  // allpad_in
	setTvtoolBankAddrVal(0x1038, 0xc2, 0x0001);   // ISP CLK=123.4Mhz
	setTvtoolBankAddrVal(0x1038, 0xc4, 0x0800);   // MCLK=12Mhz

	setTvtoolBankAddrVal(0x1309, 0xF0, 0x0101);   //reg_i2c enable
	setTvtoolBankAddrVal(0x1309, 0xF2, 0x0000);   //reg_sen_m2s_2nd_reg_adr
	setTvtoolBankAddrVal(0x1309, 0xF4, 0x0000);   //reg_sen_m2s_reg_adr_mode 8=16bit series mode ; eg_sen_m2s_cmd_bl 16bit
	setTvtoolBankAddrVal(0x1309, 0xFA, 0x0200);   //0x0038      //reg_sen_prescale

	setTvtoolBankAddrVal(0x1302, 0x20, 0x5500);
	//RST H, PWR H
	setTvtoolBankAddrVal(0x1302, 0x06, 0x0100);
	mdelay(100);
	setTvtoolBankAddrVal(0x1302, 0x06, 0x0000);
	mdelay(100);
	setTvtoolBankAddrVal(0x1302, 0x06, 0x0100);

	//read ID from 0x1c,0x1d
	setTvtoolBankAddrVal(0x1309, 0xf6, 0x1c61); //check 0x1309fe=0x7f(High byte)
	mdelay(300);
	setTvtoolBankAddrVal(0x1309, 0xf6, 0x1d61); //check 0x1309fe=0xa2(Low byte)
	mdelay(300);

	setTvtoolBankAddrVal(0x1309, 0xf8, 0x0054); //clock
	setTvtoolBankAddrVal(0x1309, 0xf2, 0x0000);
	setTvtoolBankAddrVal(0x1309, 0xf6, 0x5c60);
	mdelay(300);

	setTvtoolBankAddrVal(0x1309, 0xf8, 0x000f); //clock
	setTvtoolBankAddrVal(0x1309, 0xf2, 0x0000);
	setTvtoolBankAddrVal(0x1309, 0xf6, 0x1160);
	mdelay(300);

	//write sensor slave=0x60 , reg=0x97, data=0x0A , DISABLE COLOR BAR
	setTvtoolBankAddrVal(0x1309, 0xf8, 0x0000); //disable color bar
	setTvtoolBankAddrVal(0x1309, 0xf2, 0x0000);
	setTvtoolBankAddrVal(0x1309, 0xf6, 0x9760);
	mdelay(300);

	//write sensor slave=0x60 , reg=0x12, data=0x00 , DISABLE COLOR BAR
	setTvtoolBankAddrVal(0x1309, 0xf8, 0x0000); //disable color bar
	setTvtoolBankAddrVal(0x1309, 0xf2, 0x0000);
	setTvtoolBankAddrVal(0x1309, 0xf6, 0x1260);
	mdelay(300);

	//write sensor slave=0x60 , reg=0x97, data=0x0A , DISABLE COLOR BAR
	setTvtoolBankAddrVal(0x1309, 0xf8, 0x0000); //disable color bar
	setTvtoolBankAddrVal(0x1309, 0xf2, 0x0000);
	setTvtoolBankAddrVal(0x1309, 0xf6, 0x9760);
	mdelay(300);

#if 0
	int i;
	for (i=0;i<0x80;i++)
		pr_err("bank 0x1302, offset %#x, val = %#x\n", i, getTvtoolBankAddrVal(0x1302,i));
#endif
}
