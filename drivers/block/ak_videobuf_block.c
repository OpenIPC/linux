#include <linux/module.h>     //支持动态添加和卸载模块
#include <linux/kernel.h>    //驱动要写入内核，与内核相关的头文件
#include <linux/init.h>      //初始化头文件

#include <linux/fs.h>        //包含了文件操作相关struct的定义
#include <linux/types.h>     //对一些特殊类型的定义
#include <linux/fcntl.h>     //定义了文件操作等所用到的相关宏
#include <linux/vmalloc.h>  //vmalloc()分配的内存虚拟地址上连续，物理地址不连续
#include <linux/blkdev.h>  //采用request方式 块设备驱动程序需要调用blk_init_queue 分配请求队列
#include <linux/hdreg.h> //硬盘参数头文件，定义访问硬盘寄存器端口、状态码和分区表等信息。
#include <mach/map.h>

#define RAMHD_NAME              "video_ram"    //设备名称
#define RAMHD_MAX_DEVICE        1           //最大设备数
#define RAMHD_MAX_PARTITIONS    1           //最大分区数

#define RAMHD_SECTOR_SIZE       512        //扇区大小
#define RAMHD_SECTORS           16         //扇区数  http://www.embedu.org/Column/Column863.htm
#define RAMHD_HEADS             4         //磁头数
#define RAMHD_CYLINDERS         256      //磁道(柱面)数 

#define RAMHD_SECTOR_TOTAL      (RAMHD_SECTORS * RAMHD_HEADS * RAMHD_CYLINDERS)  //总大小
#define RAMHD_SIZE              (RAMHD_SECTOR_SIZE * RAMHD_SECTOR_TOTAL) //8MB

typedef struct{
    unsigned char   *data;             //设备数据空间首地址
    struct request_queue *queue;       //设备请求队列
    spinlock_t      lock;             //互斥自旋锁
    struct gendisk  *gd;              //通用磁盘结构体
}RAMHD_DEV;

static char *sdisk[RAMHD_MAX_DEVICE];  //分配内存的首地址
static RAMHD_DEV *rdev[RAMHD_MAX_DEVICE];  //分配内存的首地址

static dev_t ramhd_major;   //主设备号

static unsigned long disk_addr = AK_PA_VIDEO_BUFF;
static unsigned long disk_size = AK_SZ_VIDEO_BUFF;

module_param(disk_addr, ulong, S_IRUGO);
module_param(disk_size, ulong, S_IRUGO);

static int ramhd_space_init(void)
{
    int i;
    int err = 0;
    for(i = 0; i < RAMHD_MAX_DEVICE; i++){
        sdisk[i] = ioremap(disk_addr, disk_size);
        if(!sdisk[i]){
            err = -ENOMEM;  //errno:12 内存不足
            return err;
        }
		memset(sdisk[i], 0, disk_size);
    }

    return err;
}

static void ramhd_space_clean(void)
{
    int i;
    for(i = 0; i < RAMHD_MAX_DEVICE; i++){
        iounmap((void*)disk_addr);
    }
}

static int alloc_ramdev(void)
{
    int i;
    for(i = 0; i < RAMHD_MAX_DEVICE; i++){
        rdev[i] = kzalloc(sizeof(RAMHD_DEV), GFP_KERNEL); //向内核申请存放RAMHD_DEV结构体的内存空间
        if(!rdev[i])
            return -ENOMEM;   //errno:12  内存不足
    }
    return 0;
}

static void clean_ramdev(void)
{
    int i;
    for(i = 0; i < RAMHD_MAX_DEVICE; i++){
        if(rdev[i])
            kfree(rdev[i]);   //释放分配的内存
    }   
}       

int ramhd_open(struct block_device *bdev, fmode_t mode)   //设备打开用到
{   
    return 0;
}

int ramhd_release(struct gendisk *gd, fmode_t mode)   //设备关闭用到
{   
    return 0;
}

static int ramhd_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg) //IO控制
{
    int err;
    struct hd_geometry geo;  //hd_geometry结构体包含磁头，扇区，柱面等信息

    switch(cmd)
    {
        case HDIO_GETGEO:  //获取块设备的物理参数 
            err = !access_ok(VERIFY_WRITE, arg, sizeof(geo));//检查指针所指向的存储块是否可写
            if(err) return -EFAULT;     //errno:14   地址错 
        
            geo.cylinders = RAMHD_CYLINDERS;     //柱面数
            geo.heads = RAMHD_HEADS;            //磁头数
            geo.sectors = RAMHD_SECTORS;        //扇区数
            geo.start = get_start_sect(bdev);   //起始地址
            if(copy_to_user((void *)arg, &geo, sizeof(geo)))
             //把内核地址&geo指示的数据复制到arg指代的用户空间的地址上
                return -EFAULT;   //errno:14   地址错 
            return 0;
    }
            
    return -ENOTTY;      //errno:25     不适当的IO控制操作 
}

static struct block_device_operations ramhd_fops =  //用来描述一个块设备的操作函数集
{   
	.owner = THIS_MODULE,
    .open = ramhd_open,
    .release = ramhd_release,
    .ioctl = ramhd_ioctl,
};

void ramhd_req_func (struct request_queue *q)  //处理传递给这个设备的请求
{
    struct request *req;  //用来提取req
	RAMHD_DEV *pdev;
	char *pData;
	unsigned long addr, size, start;
	req = blk_fetch_request(q); //从块设备队列提取存储的req;
        //blk_fetch_request()可以多次调用，如果queue里面没有内容，req将返回NULL
	while (req) {   //判断当前request是否合法  循环从请求队列中获取下一个要处理的请求
		start = blk_rq_pos(req); // 获取当前request结构的起始扇区 
		pdev = (RAMHD_DEV *)req->rq_disk->private_data; //获得设备结构体指针
		pData = pdev->data;//设备地址
		addr = (unsigned long)pData + start * RAMHD_SECTOR_SIZE;//计算地址
        size = blk_rq_cur_bytes(req); //访问 req 的下一段数据
		if (rq_data_dir(req) == READ) //获得数据传送方向.返回0表示从设备读取,否则表示写向设备.
			memcpy(req->buffer, (char *)addr, size); //读
		else
			memcpy((char *)addr, req->buffer, size); //写
	
		if(!__blk_end_request_cur(req, 0))  //这个函数处理完返回false
			req = blk_fetch_request(q);  //继续取出请求队列中的请求
	}
}

int ramhd_init(void)   //初始化
{
    int i;
	int ret;

	if((disk_addr < 0x80000000) || ((disk_addr + disk_size)  > (0x80000000 + AK_SZ_VIDEO_BUFF))){
		printk(KERN_ERR "###error!\n");
		printk(KERN_ERR "disk_addr + disk_size must :0x80000000 - %x!\n", 0x80000000 + AK_SZ_VIDEO_BUFF);
		return -1;
	}
	
    ret = ramhd_space_init();
	if(ret < 0){
		printk(KERN_ERR "###ramhd_space_init fail!\n");
		return -1;
	}
	
    ret = alloc_ramdev();
	if(ret < 0){
		printk(KERN_ERR "###alloc_ramdev fail!\n");
		return -1;
	}
    
    ramhd_major = register_blkdev(0, RAMHD_NAME); //块设备驱动注册到内核中
    //major为0，内核会自动分配一个新的主设备号（ramhd_major ）  
    for(i = 0; i < RAMHD_MAX_DEVICE; i++)
    {
        rdev[i]->data = sdisk[i]; 
        rdev[i]->gd = alloc_disk(RAMHD_MAX_PARTITIONS);
        spin_lock_init(&rdev[i]->lock);  //初始化自旋锁
        rdev[i]->queue = blk_init_queue(ramhd_req_func, &rdev[i]->lock);//初始化将ramhd_req_func函数与队列绑定
        rdev[i]->gd->major = ramhd_major; 
        rdev[i]->gd->first_minor = i * RAMHD_MAX_PARTITIONS;
        rdev[i]->gd->fops = &ramhd_fops;  //关联到这个设备的方法集合
        rdev[i]->gd->queue = rdev[i]->queue;
        rdev[i]->gd->private_data = rdev[i]; //使用这个成员来指向分配的数据
        sprintf(rdev[i]->gd->disk_name, "video_ram%d", i);
        //set_capacity(rdev[i]->gd, RAMHD_SECTOR_TOTAL);
        set_capacity(rdev[i]->gd, disk_size/RAMHD_SECTOR_SIZE);
        add_disk(rdev[i]->gd);   //向系统中添加这个块设备
    }
        
    return 0;
}

void ramhd_exit(void)  //模块卸载函数
{
    int i;
    for(i = 0; i < RAMHD_MAX_DEVICE; i++)
    {
        del_gendisk(rdev[i]->gd); //删除gendisk结构体   
        put_disk(rdev[i]->gd);   //减少gendisk结构体的引用计数
        blk_cleanup_queue(rdev[i]->queue);  //清除请求对列
    }
    unregister_blkdev(ramhd_major,RAMHD_NAME); ; //注销块设备 
    clean_ramdev();
    ramhd_space_clean();  
}

module_init(ramhd_init);
module_exit(ramhd_exit);


MODULE_AUTHOR("lyl");
MODULE_DESCRIPTION("The Ramdisk implementation with request function");
MODULE_LICENSE("GPL");
