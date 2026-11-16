#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <soc/base.h>


extern int jz_lzma_decompress(unsigned char *src, size_t size, unsigned char *dst);
//extern int jz_lzma_decompress(void);

int unlzma_hardware(unsigned char *input,size_t in_len,unsigned char *output)
{
    int ret_icl;

    ret_icl=jz_lzma_decompress(input,in_len,output);
    if(ret_icl < 0){
        printk("lzma_ioctl error\n");
        return -1;
    }

    return 0;

}
