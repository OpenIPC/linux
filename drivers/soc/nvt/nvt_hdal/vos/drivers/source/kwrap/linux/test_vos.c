#define __MODULE__    test_vos
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

#ifndef __LINUX_USER
#include <kwrap/cpu.h>
#include <kwrap/dev.h>
#include <kwrap/mem.h>
#endif

#include <kwrap/file.h>
#include <kwrap/flag.h>
#include <kwrap/perf.h>
#include <kwrap/semaphore.h>
#include <kwrap/spinlock.h>
#include <kwrap/stdio.h>
#include <kwrap/task.h>
#include <kwrap/util.h>

#ifdef __FREERTOS
#include <stdio.h>
#include <string.h>
#include <kwrap/examsys.h>
#include <kwrap/mailbox.h>
#endif

#ifdef __LINUX
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#endif

#ifdef __LINUX_USER
#include <string.h>
#include <kwrap/mailbox.h>
#endif

#define FLGPTN_XFLAG_BIT0   FLGPTN_BIT(0)
#define FLGPTN_XFLAG_BIT1   FLGPTN_BIT(1)

#define FLGPTN_DONE_TASK1    FLGPTN_BIT(0)
#define FLGPTN_DONE_TASK2    FLGPTN_BIT(1)
#define FLGPTN_DONE_TASK3    FLGPTN_BIT(2)
#define FLGPTN_DONE_TASK4    FLGPTN_BIT(3)

static ID xSemaphore1; //old usage: static SEM_HANDLE xSemaphore1;
static ID xSemaphore2; //old usage: static SEM_HANDLE xSemaphore2;
static ID xSemTaskDone;

static ID xFlag1; //old usage: static ID xFlag1
static ID xFlagTask3;
static ID xFlagTask4;
static ID xFlagTaskDone;

static VK_TASK_HANDLE g_tskhdl_spin1;
static VK_TASK_HANDLE g_tskhdl_spin2;
static THREAD_HANDLE xTaskHdl1;
static THREAD_HANDLE xTaskHdl2;
static THREAD_HANDLE xTaskHdl3;
static THREAD_HANDLE xTaskHdl4;
static THREAD_HANDLE xTaskHdl5;
static THREAD_HANDLE xTaskHdl6;

typedef struct {
	int val;
	char name[16];
} MBX_DATA;

THREAD_RETTYPE sem_task1( void * pvParameters )
{
	int num_loop = 10;

	if (xTaskHdl1 != vos_task_get_handle()) {
		DBG_ERR("get handle failed\r\n");
	} else {
		CHAR name[32] = {0};
		vos_task_get_name(xTaskHdl1, name, sizeof(name));
		DBG_DUMP("vos_task_get_name = %s\r\n", name);
	}

	vos_perf_list_mark(__func__, __LINE__, 0);

	while (num_loop--) {
		vos_sem_wait(xSemaphore1);//old usage: SEM_WAIT(xSemaphore1);
		DBG_DUMP("I am %s\n", __func__);
		vos_sem_sig(xSemaphore2);//old usage: SEM_SIGNAL(xSemaphore2);
		//vos_util_delay_ms(100);
	}

	vos_perf_list_mark(__func__, __LINE__, 0);
	vos_sem_sig(xSemTaskDone);
	THREAD_RETURN(0);
}

THREAD_RETTYPE sem_task2( void * pvParameters )
{
	int num_loop = 10;

	vos_perf_list_mark(__func__, __LINE__, 0);

	while (num_loop--) {
		vos_sem_wait(xSemaphore2); //old usage: SEM_WAIT(xSemaphore2);
		DBG_DUMP("I am %s\n", __func__);
		vos_sem_sig(xSemaphore1); //old usage: SEM_SIGNAL(xSemaphore1);
		//vos_util_delay_ms(100);
	}

	vos_perf_list_mark(__func__, __LINE__, 0);
	vos_sem_sig(xSemTaskDone);
	THREAD_RETURN(0);
}

THREAD_RETTYPE flag_task1( void * pvParameters )
{
	int num_loop = 10;
	FLGPTN flgptn;

	vos_perf_list_mark(__func__, __LINE__, 0);

	while (num_loop--) {
		wai_flg(&flgptn, xFlag1, FLGPTN_XFLAG_BIT0, TWF_ORW | TWF_CLR);
		DBG_DUMP("I am %s\r\n", __func__);
		set_flg(xFlag1, FLGPTN_XFLAG_BIT1);
	}

	vos_perf_list_mark(__func__, __LINE__, 0);

	set_flg(xFlagTaskDone, FLGPTN_DONE_TASK1);
	THREAD_RETURN(0);
}

THREAD_RETTYPE flag_task2( void * pvParameters )
{
	int num_loop = 10;
	FLGPTN flgptn;
	VOS_TICK t1 = 0, t2 = 0;

	vos_perf_list_mark(__func__, __LINE__, 0);

	vos_perf_mark(&t1);
	while (num_loop--) {
		wai_flg(&flgptn, xFlag1, FLGPTN_XFLAG_BIT1, TWF_ORW | TWF_CLR);
		DBG_DUMP("I am %s\r\n", __func__);
		set_flg(xFlag1, FLGPTN_XFLAG_BIT0);
	}
	vos_perf_mark(&t2);
	DBG_DUMP("%s: t1 %d, t2 %d, duration %d\r\n", __func__, (unsigned int)t1, (unsigned int)t2, (unsigned int)vos_perf_duration(t1, t2));

	vos_perf_list_mark(__func__, __LINE__, 0);
	vos_perf_list_dump();

	set_flg(xFlagTaskDone, FLGPTN_DONE_TASK2);
	THREAD_RETURN(0);
}

THREAD_RETTYPE flag_task3( void * pvParameters )
{
	int bit_num;
	FLGPTN retptn;
	FLGPTN waiptn;
	FLGPTN chkptn;

	DBG_DUMP("%s test begin\r\n", __func__);

	for (bit_num = 0; bit_num < 32; bit_num++) {
		waiptn = FLGPTN_BIT(bit_num);

		//set to trigger task4
		if (0 != vos_flag_set(xFlagTask4, waiptn)) {
			DBG_DUMP("%s vos_flag_set failed\r\n", __func__);
			break;
		}

		//wait task4 to set back
		if (0 != vos_flag_wait(&retptn, xFlagTask3, waiptn, TWF_ORW)) {
			DBG_DUMP("%s vos_flag_wait failed\r\n", __func__);
			break;
		}
		#if 0 //for debug
		DBG_DUMP("%s: bit_num %d, retptn 0x%X\r\n", __func__, bit_num, retptn);
		#endif

		//check the flag status
		if (0 == (waiptn & retptn)) {
			DBG_DUMP("%s: waiptn 0x%X, retptn 0x%X, bit not set\r\n", __func__, waiptn, retptn);
			break;
		}

		if (0 != vos_flag_clr(xFlagTask3, waiptn)) {
			DBG_DUMP("%s vos_flag_clr failed\r\n", __func__);
			break;
		}

		chkptn = vos_flag_chk(xFlagTask3, waiptn);
		if (chkptn & waiptn) {
			DBG_DUMP("%s bit not cleared\r\n", __func__);
			break;
		}
	}

	chkptn = vos_flag_chk(xFlagTask3, FLGPTN_BIT_ALL);
	if (0 != chkptn) {
		DBG_DUMP("%s chkptn 0x%X should be 0\r\n", __func__, chkptn);
	}

	DBG_DUMP("%s test end\r\n", __func__);

	set_flg(xFlagTaskDone, FLGPTN_DONE_TASK3);
	THREAD_RETURN(0);
}

THREAD_RETTYPE flag_task4( void * pvParameters )
{
	int bit_num;
	FLGPTN retptn;
	FLGPTN waiptn;
	FLGPTN chkptn;

	DBG_DUMP("%s test begin\r\n", __func__);

	for (bit_num = 0; bit_num < 32; bit_num++) {
		waiptn = FLGPTN_BIT(bit_num);

		//wait task3 to set flag
		if (0 != vos_flag_wait(&retptn, xFlagTask4, waiptn, TWF_ORW | TWF_CLR)) {
			DBG_DUMP("%s vos_flag_wait failed\r\n", __func__);
			break;
		}
		#if 0 //for debug
		DBG_DUMP("%s: bit_num %d, retptn 0x%X\r\n", __func__, bit_num, retptn);
		#endif

		//check the flag status
		if (waiptn != retptn) {
			DBG_DUMP("%s: waiptn 0x%X, retptn 0x%X, bit not cleared\r\n", __func__, waiptn, retptn);
		}

		chkptn = vos_flag_chk(xFlagTask3, waiptn);
		if (chkptn & waiptn) {
			DBG_DUMP("%s bit not cleared\r\n", __func__);
			break;
		}

		//set to trigger task3
		if (0 != vos_flag_set(xFlagTask3, waiptn)) {
			DBG_DUMP("%s vos_flag_set failed\r\n", __func__);
			break;
		}
	}

	chkptn = vos_flag_chk(xFlagTask4, FLGPTN_BIT_ALL);
	if (0 != chkptn) {
		DBG_DUMP("%s chkptn 0x%X should be 0\r\n", __func__, chkptn);
	}

	DBG_DUMP("%s test end\r\n", __func__);

	set_flg(xFlagTaskDone, FLGPTN_DONE_TASK4);
	THREAD_RETURN(0);
}

#ifndef __LINUX
static ID xMailbox1;

THREAD_RETTYPE mbx_snd_task(void *ptr)
{
	MBX_DATA snd_data = {0, "DATA_MBX"};
	int num_loop = 10;

	DBG_FUNC_BEGIN("\r\n");
	vos_task_enter(); //kent_tsk()

	while(num_loop--) {
		DBG_DUMP("I am %s, SND: val %d, name %s\r\n", __func__, snd_data.val, snd_data.name);
		if (E_OK != vos_mbx_snd(xMailbox1, &snd_data, sizeof(snd_data))) {
			DBG_ERR("vos_mbx_snd failed\r\n");
			break;
		}
		snd_data.val++;
	}

	DBG_FUNC_END("\r\n");

	THREAD_RETURN(0);
}

THREAD_RETTYPE mbx_rcv_task(void *ptr)
{
	MBX_DATA rcv_data = {0};
	int num_loop = 10;

	DBG_FUNC_BEGIN("\r\n");
	vos_task_enter(); //kent_tsk()

	while(num_loop--) {
		if (E_OK != vos_mbx_rcv(xMailbox1, &rcv_data, sizeof(rcv_data))) {
			DBG_ERR("vos_mbx_rcv failed\r\n");
			break;
		}
		DBG_DUMP("I am %s, RCV: val %d, name %s, is_empty %d\r\n", __func__, rcv_data.val, rcv_data.name, vos_mbx_is_empty(xMailbox1));
	}

	DBG_FUNC_END("\r\n");

	THREAD_RETURN(0);
}

int mbx_test(void)
{
	THREAD_HANDLE task1_hdl, task2_hdl;
	VOS_MBX_PARAM mbx_param = {10, sizeof(MBX_DATA)};

	DBG_FUNC_BEGIN("\n");

#ifdef __LINUX_USER
	if (E_OK != vos_mbx_init(8)) {
		DBG_ERR("vos_mbx_init failed\r\n");
		goto testfunc_mailbox_exit;
	}
#endif

	DBG_DUMP("vos_mbx_create maxmsg %d, msgsize %d\r\n", mbx_param.maxmsg, mbx_param.msgsize);
	if (E_OK != vos_mbx_create(&xMailbox1, &mbx_param)) {
		DBG_ERR("vos_mbx_create failed\r\n");
		goto testfunc_mailbox_exit;
	}

	task1_hdl = vos_task_create(mbx_snd_task, NULL, "mbx_snd_task", 10, 8*1024);
	if (0 == task1_hdl) {
		DBG_ERR("vos_task_create failed\r\n");
		goto testfunc_mailbox_exit;
	}
	vos_task_resume(task1_hdl);

	task2_hdl = vos_task_create(mbx_rcv_task, NULL, "mbx_rcv_task", 10, 8*1024);
	if (0 == task2_hdl) {
		DBG_ERR("vos_task_create failed\r\n");
		goto testfunc_mailbox_exit;
	}
	vos_task_resume(task2_hdl);

	vos_util_delay_ms(1000);

	vos_mbx_destroy(xMailbox1);

testfunc_mailbox_exit:
#ifdef __LINUX_USER
	if (E_OK != vos_mbx_exit()) {
		DBG_ERR("vos_mbx_exit failed\r\n");
	}
#endif
	DBG_FUNC_END("\n");
	return 0;
}
#endif //#ifdef __FREERTOS

#ifdef __LINUX
static int dev_fops_open(struct inode *inode, struct file *file)
{
	printk("MAJOR(%d), MINOR(%d)\r\n", imajor(inode), iminor(inode));
	return 0;
}

ssize_t dev_fops_read(struct file *filp, char __user *buf, size_t count, loff_t *offp)
{
	return 0;
}

static struct file_operations dev_fops = {
	.owner      = THIS_MODULE,
	.open       = dev_fops_open,
	.read       = dev_fops_read,
};

static int cdev_unreg_cb(struct device *dev, void *data)
{
	device_unregister(dev);
	return 0;
}

static int cdev_reg_test(dev_t devid, unsigned count, const char* class_name)
{
	struct cdev *p_cdev = NULL;
	struct class *p_class = NULL;
	struct device *p_device = NULL;
	int dev_no;
	int ret = 0;

	p_cdev = cdev_alloc();
	if (NULL == p_cdev) {
		DBG_ERR("cdev_alloc failed\r\n");
		ret = -1;
		goto cdev_reg_test_end;
	}
	p_cdev->ops = &dev_fops;

	ret = cdev_add(p_cdev, devid, count);
	if (0 != ret) {
		DBG_ERR(" cdev_add failed\n");
		ret = -1;
		goto cdev_reg_test_end;
	}

	p_class = class_create(THIS_MODULE, class_name);
	if (IS_ERR(p_class)) {
		DBG_ERR("class_create failed\n");
		ret = -1;
		goto cdev_reg_test_end;
	}

	for (dev_no = 0; dev_no < count; dev_no++) {
		p_device = device_create(p_class, NULL, MKDEV(MAJOR(devid), MINOR(devid) + dev_no), NULL, "%s_cdev%d", class_name, dev_no);
		if (IS_ERR(p_device)) {
			DBG_ERR("device_create failed\n");
			ret = -1;
			goto cdev_reg_test_end;
		}
	}

	#if 0 //set 1 for debug, do not release to check the result
	DBG_ERR("debug mode, class will not be released\r\n");
	return ret;
	#endif

cdev_reg_test_end:
	if (p_class) {
		class_for_each_device(p_class, NULL, NULL, cdev_unreg_cb);
		class_destroy(p_class);
	}

	if (p_cdev) {
		cdev_del(p_cdev);
	}

	return ret;
}

static int dev_test(void)
{
	dev_t dev_1st = 0, dev_2nd = 0;
	unsigned int major_1st, major_2nd;
	unsigned int minor_1st, minor_2nd;
	unsigned count_1st, count_2nd;

	count_1st = 2;
	count_2nd = 3;

	//1st alloc region
	if (0 != vos_alloc_chrdev_region(&dev_1st, count_1st, "vos_dev_reg1")) {
		DBG_ERR("vos_alloc_chrdev_region 1 failed\r\n");
	}
	major_1st = MAJOR(dev_1st);
	minor_1st = MINOR(dev_1st);
	DBG_DUMP("major_1st %d, minor_1st %d, count_1st %d\r\n", major_1st, minor_1st, count_1st);

	//2nd alloc region
	if (0 != vos_alloc_chrdev_region(&dev_2nd, count_2nd, "vos_dev_reg2")) {
		DBG_ERR("vos_alloc_chrdev_region 2 failed\r\n");
	}
	major_2nd = MAJOR(dev_2nd);
	minor_2nd = MINOR(dev_2nd);
	DBG_DUMP("major_2nd %d, minor_2nd %d, count_2nd %d\r\n", major_2nd, minor_2nd, count_2nd);

	//check alloc results
	if (major_1st != major_2nd) {
		DBG_ERR("major not matched, 1st(%d) 2nd(%d)\r\n", major_1st, major_2nd);
	}

	if ((minor_1st + count_1st) != minor_2nd) {
		DBG_ERR("minor not continuous, 1st(%d) 2nd(%d)\r\n", minor_1st, minor_2nd);
	}

	//try to register device node and release right away
	if (0 != cdev_reg_test(dev_1st, count_1st, "vos_class1")) {
		DBG_ERR("cdev_reg 1 failed\r\n");
	}

	if (0 != cdev_reg_test(dev_2nd, count_2nd, "vos_class2")) {
		DBG_ERR("cdev_reg 2 failed\r\n");
	}

	//unregister region
	vos_unregister_chrdev_region(dev_1st, count_1st);
	vos_unregister_chrdev_region(dev_2nd, count_2nd);

	return 0;
}
#endif //#ifdef __LINUX

static void print_dbg_msg(void)
{
	int dec_val = 12345678;
	int hex_val = 0x12345678;
	DBG_DUMP("pring dbg msg -------- begin\r\n");
	DBG_FATAL("my fatal\r\n");
	DBG_ERR("my error\r\n");
	DBG_WRN("my warning\r\n");
	DBG_UNIT("my unit\r\n");
	DBG_FUNC_BEGIN("my func\r\n");
	DBG_FUNC("my func\r\n");
	DBG_FUNC_END("my func\r\n");
	DBG_IND("my ind\r\n");
	DBG_MSG("my msg\r\n");
	DBG_VALUE("my value\r\n");
	DBG_USER("my user\r\n");
	DBG_DUMP("my dump\r\n");
	CHKPNT;
	DBGD(dec_val);
	DBGH(hex_val);
	DBG_DUMP("pring dbg msg -------- end\r\n");
}

#ifndef __LINUX_USER
static int mem_cma_test(void)
{
	struct vos_mem_cma_info_t cma_info = {0};
	VOS_MEM_CMA_HDL cma_hdl;

	//cache
	DBG_DUMP("test cache memory\r\n");
	if (0 != vos_mem_init_cma_info(&cma_info, VOS_MEM_CMA_TYPE_CACHE, 2048)) {
		DBG_ERR("init cma failed\r\n");
		return -1;
	}

	cma_hdl = vos_mem_alloc_from_cma(&cma_info);
	if (NULL == cma_hdl) {
		DBG_ERR("alloc cma failed\r\n");
		return -1;
	}
	DBG_DUMP("cma_info.paddr 0x%lX\r\n", (ULONG)cma_info.paddr);
	DBG_DUMP("cma_info.vaddr 0x%lX\r\n", (ULONG)cma_info.vaddr);
	DBG_DUMP("cma_info.size 0x%lX\r\n", (ULONG)cma_info.size);

	if (vos_cpu_get_phy_addr(cma_info.vaddr) != cma_info.paddr) {
		DBG_ERR("phy_addr conv failed, 0x%lX != 0x%lX\r\n", (ULONG)vos_cpu_get_phy_addr(cma_info.vaddr), (ULONG)cma_info.paddr);
	}

	memset((void *)cma_info.vaddr, 0x55, cma_info.size);

	if (0 != vos_mem_release_from_cma(cma_hdl)) {
		DBG_ERR("release cma failed\r\n");
		return -1;
	}

	//non-cache
	DBG_DUMP("test non-cache memory\r\n");
	if (0 != vos_mem_init_cma_info(&cma_info, VOS_MEM_CMA_TYPE_NONCACHE, 2048)) {
		DBG_ERR("init cma failed\r\n");
		return -1;
	}

	cma_hdl = vos_mem_alloc_from_cma(&cma_info);
	if (NULL == cma_hdl) {
		DBG_ERR("alloc cma failed\r\n");
		return -1;
	}
	DBG_DUMP("cma_info.paddr 0x%lX\r\n", (ULONG)cma_info.paddr);
	DBG_DUMP("cma_info.vaddr 0x%lX\r\n", (ULONG)cma_info.vaddr);
	DBG_DUMP("cma_info.size 0x%lX\r\n", (ULONG)cma_info.size);

	if (vos_cpu_get_phy_addr(cma_info.vaddr) != cma_info.paddr) {
		DBG_ERR("phy_addr conv failed, 0x%lX != 0x%lX\r\n", (ULONG)vos_cpu_get_phy_addr(cma_info.vaddr), (ULONG)cma_info.paddr);
	}

	memset((void *)cma_info.vaddr, 0xAA, cma_info.size);

	if (0 != vos_mem_release_from_cma(cma_hdl)) {
		DBG_ERR("release cma failed\r\n");
		return -1;
	}

	return 0;
}
#endif

static int sem_test(void)
{
	int task_num = 2;
	int ret;

	//old usage: SEM_CREATE(xSemaphore1, 1);
	if (0 != vos_sem_create(&xSemaphore1, 1, "my_xSemaphore1")) {
		return -1;
	}
	if (0 != vos_sem_create(&xSemaphore2, 1, "my_xSemaphore2")) {
		return -1;
	}
	if (0 != vos_sem_create(&xSemTaskDone, 0, "xSemTaskDone")) {
		return -1;
	}
	vos_sem_wait(xSemaphore1); //old usage: SEM_WAIT(xSemaphore1);

	vos_perf_list_reset();

	//test vos_util_msec_to_tick and use vos_perf to check
	vos_perf_list_mark(__func__, __LINE__, 0);
	ret = vos_sem_wait_timeout(xSemaphore1, vos_util_msec_to_tick(55));
	vos_perf_list_mark(__func__, __LINE__, 0);

	if (ret != E_TMOUT) {
		DBG_ERR("vos_sem_wait_timeout failed\r\n");
	} else {
		DBG_DUMP("vos_sem_wait_timeout, ret 0x%X\r\n", ret);
	}

	vos_perf_list_dump();
	vos_perf_list_reset();

	//old usage: THREAD_CREATE(xTaskHdl1, sem_task1, NULL, "TestRunner1");
	xTaskHdl1 = vos_task_create(sem_task1, NULL, "sem_task1", 10, 4*1024);
	if (0 == xTaskHdl1) {
		DBG_DUMP("create sem_task1 failed\r\n");
		return -1;
	}
	vos_task_resume(xTaskHdl1);
	vos_task_set_priority(xTaskHdl1, 10);

	//old usage: THREAD_CREATE(xTaskHdl2, sem_task2, NULL, "TestRunner2");
	xTaskHdl2 = vos_task_create(sem_task2, NULL, "sem_task2", 10, 4*1024);
	if (0 == xTaskHdl2) {
		DBG_DUMP("create sem_task2 failed\r\n");
		return -1;
	}
	vos_task_resume(xTaskHdl2);

	vos_perf_list_dump();

	//wait task done
	while(task_num--) {
		vos_sem_wait(xSemTaskDone);
	}

	vos_sem_destroy(xSemaphore1);
	vos_sem_destroy(xSemaphore2);
	vos_sem_destroy(xSemTaskDone);

#ifndef __LINUX_USER
	DBG_WRN("sem error check -------- begin\r\n");
	vos_sem_destroy(99999);
	if (0 != vos_sem_wait(99999)) {
		DBG_DUMP("intentional error\r\n");
	}
	if (0 != vos_sem_wait_timeout(99999, 100)) {
		DBG_DUMP("intentional error\r\n");
	}
	if (0 != vos_sem_wait_interruptible(99999)) {
		DBG_DUMP("intentional error\r\n");
	}
	vos_sem_sig(99999);
	DBG_WRN("sem error check -------- end\r\n");
#endif

	return 0;
}

static int flag_test(void)
{
	FLGPTN retptn = 0;
	FLGPTN waiptn = FLGPTN_DONE_TASK1 | FLGPTN_DONE_TASK2 | FLGPTN_DONE_TASK3 | FLGPTN_DONE_TASK4;
	int ret;

	//old usage: OS_CONFIG_FLAG(xFlag1);
	if (E_OK != cre_flg(&xFlag1, NULL, "xFlag1")) {
		return -1;
	}
	if (E_OK != cre_flg(&xFlagTask3, NULL, "xFlagTask3")) {
		return -1;
	}
	if (E_OK != cre_flg(&xFlagTask4, NULL, "xFlagTask4")) {
		return -1;
	}
	if (E_OK != cre_flg(&xFlagTaskDone, NULL, "xFlagTaskDone")) {
		return -1;
	}

	vos_perf_list_reset();

	//test vos_util_msec_to_tick and use vos_perf to check
	vos_perf_list_mark(__func__, __LINE__, 0);
	ret = vos_flag_wait_timeout(&retptn, xFlag1, waiptn, TWF_ANDW, vos_util_msec_to_tick(100));
	vos_perf_list_mark(__func__, __LINE__, 0);

	if (ret != E_TMOUT)
		DBG_ERR("vos_flag_wait_timeout failed\r\n");
	else {
		DBG_DUMP("vos_flag_wait_timeout, ret 0x%X\r\n", ret);
	}

	vos_perf_list_dump();
	vos_perf_list_reset();

	set_flg(xFlag1, FLGPTN_XFLAG_BIT0);

	//old usage: THREAD_CREATE(xTaskHdl3, flag_task1, NULL, "TestRunner3");
	xTaskHdl3 = vos_task_create(flag_task1, NULL, "flag_task1", 10, 4*1024);
	if (0 == xTaskHdl3) {
		DBG_DUMP("create flag_task1 failed\r\n");
		return -1;
	}
	vos_task_resume(xTaskHdl3);

	//old usage: THREAD_CREATE(xTaskHdl4, flag_task2, NULL, "TestRunner4");
	xTaskHdl4 = vos_task_create(flag_task2, NULL, "flag_task2", 10, 4*1024);
	if (0 == xTaskHdl4) {
		DBG_DUMP("create flag_task2 failed\r\n");
		return -1;
	}
	vos_task_resume(xTaskHdl4);

	xTaskHdl5 = vos_task_create(flag_task3, NULL, "flag_task3", 10, 4*1024);
	if (0 == xTaskHdl5) {
		DBG_DUMP("create flag_task3 failed\r\n");
		return -1;
	}
	vos_task_resume(xTaskHdl5);

	xTaskHdl6 = vos_task_create(flag_task4, NULL, "flag_task4", 10, 4*1024);
	if (0 == xTaskHdl6) {
		DBG_DUMP("create flag_task4 failed\r\n");
		return -1;
	}
	vos_task_resume(xTaskHdl6);

	vos_flag_wait(&retptn, xFlagTaskDone, waiptn, TWF_ANDW);
	vos_flag_destroy(xFlag1);
	vos_flag_destroy(xFlagTask3);
	vos_flag_destroy(xFlagTask4);
	vos_flag_destroy(xFlagTaskDone);

#ifndef __LINUX_USER
	{
		FLGPTN dummyptn = 0;
		DBG_WRN("flag error check -------- begin\r\n");
		vos_flag_destroy(99999);
		vos_flag_set(99999, dummyptn);
		if (0 != vos_flag_clr(99999, dummyptn)) {
			DBG_DUMP("intentional error\r\n");
		}
		if (E_OK != vos_flag_wait(&retptn, 99999, waiptn, TWF_ORW)) {
			DBG_DUMP("intentional error\r\n");
		}
		if (E_OK != vos_flag_wait_timeout(&retptn, 99999, waiptn, TWF_ORW, 100)) {
			DBG_DUMP("intentional error\r\n");
		}
		if (0 == vos_flag_chk(99999, dummyptn)) {
			DBG_DUMP("intentional error\r\n");
		}
		DBG_WRN("flag error check -------- end\r\n");
	}
#endif

	return 0;
}

int file_test(void)
{
	int ret = 0;
	VOS_FILE fd;
	int rw_bytes;
	char write_buf[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char read_buf[64] = {0};
	off_t offset;
	struct vos_stat statbuf;

	//linux style path => /mnt/sd
	fd = vos_file_open("/mnt/sd/test.txt", O_RDWR|O_CREAT|O_TRUNC, 666);
	if (VOS_FILE_INVALID == fd) {
		DBG_ERR("vos_file_open wr fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	DBG_DUMP("vos_file_open write fd = 0x%lX\r\n", (ULONG)fd);

	rw_bytes = vos_file_write(fd, write_buf, sizeof(write_buf));
	if (sizeof(write_buf) != rw_bytes) {
		DBG_ERR("vos_file_write fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	DBG_DUMP("vos_file_write %d bytes, data [%s]\r\n", rw_bytes, write_buf);

	if (-1 == vos_file_fsync(fd)) {
		DBG_ERR("vos_file_fsync fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	DBG_DUMP("vos_file_fsync ok\r\n");

	if (-1 == vos_file_close(fd)) {
		DBG_ERR("vos_file_close fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}

#ifdef __FREERTOS
	//uitron style path => A:
	fd = vos_file_open("A:\\test.txt", O_RDONLY, 666);
#else
	fd = vos_file_open("/mnt/sd/test.txt", O_RDONLY, 666);
#endif
	if (VOS_FILE_INVALID == fd) {
		DBG_ERR("vos_file_open read fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	DBG_DUMP("vos_file_open read fd = 0x%lX, read_buf 0x%lX\r\n", (ULONG)fd, (ULONG)read_buf);

	rw_bytes = vos_file_read(fd, read_buf, sizeof(read_buf));
	if (-1 == rw_bytes) {
		DBG_ERR("vos_file_read fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	DBG_DUMP("vos_file_read %d bytes, data [%s]\r\n", rw_bytes, read_buf);

	offset = vos_file_lseek(fd, 0, SEEK_END);
	if ((off_t)(-1) == ret) {
		DBG_ERR("vos_file_lseek fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	DBG_DUMP("vos_file_lseek end, current offset %ld\r\n", offset);

	offset = vos_file_lseek(fd, 7, SEEK_SET);
	if ((off_t)(-1) == ret) {
		DBG_ERR("vos_file_lseek fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	DBG_DUMP("vos_file_lseek 7 from head, current offset %ld\r\n", offset);

	rw_bytes = vos_file_read(fd, read_buf, sizeof(read_buf));
	if (-1 == rw_bytes) {
		DBG_ERR("vos_file_read fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	DBG_DUMP("vos_file_read %d bytes, data [%s]\r\n", rw_bytes, read_buf);

	if (-1 == vos_file_fstat(fd, &statbuf)) {
		DBG_ERR("vos_file_fstat fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	DBG_DUMP("vos_file_fstat size = %ld, mode 0x%X, ISDIR %d\r\n", statbuf.st_size, statbuf.st_mode, S_ISDIR(statbuf.st_mode));

	if (-1 == vos_file_close(fd)) {
		DBG_ERR("vos_file_close fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	fd = VOS_FILE_INVALID; //fd is closed

	memset(&statbuf, 0, sizeof(statbuf));
	if (-1 == vos_file_stat("/mnt/sd/test.txt", &statbuf)) {
		DBG_ERR("vos_file_stat fail\r\n");
		ret = -1;
		goto test_vos_file_end;
	}
	DBG_DUMP("vos_file_stat size = %ld, mode 0x%X, ISDIR %d\r\n", statbuf.st_size, statbuf.st_mode, S_ISDIR(statbuf.st_mode));

test_vos_file_end:
	if (VOS_FILE_INVALID != fd) {
		if (-1 == vos_file_close(fd)) {
			DBG_ERR("vos_file_close fail\r\n");
			ret = -1;
		}
	}

	return ret;
}

static int sscanf_s_test(const char* str)
{
	char path[16] = {0};
	int val = 0;

	sscanf_s(str, "%s %d", path, sizeof(path), &val);
	DBG_DUMP("sscanf_s path [%s] val %d\r\n", path, val);

	return 0;
}

static VK_DEFINE_SPINLOCK(g_test_lock);

THREAD_RETTYPE spin_task1(void *param)
{
	int num_loop = 10;
	unsigned long flags;

	while (num_loop--) {
		vk_spin_lock_irqsave(&g_test_lock, flags);
		DBG_DUMP("I am %s\n", __func__);
		vk_spin_unlock_irqrestore(&g_test_lock, flags);
	}

	THREAD_RETURN(0);
}

THREAD_RETTYPE spin_task2(void *param)
{
	int num_loop = 10;
	unsigned long flags;

	while (num_loop--) {
		vk_spin_lock_irqsave(&g_test_lock, flags);
		DBG_DUMP("I am %s\n", __func__);
		vk_spin_unlock_irqrestore(&g_test_lock, flags);
	}

	THREAD_RETURN(0);
}

static int spinlock_test(void)
{
	DBG_WRN("vk_spinlock_t size = %ld\r\n", (ULONG)sizeof(vk_spinlock_t));

	g_tskhdl_spin1 = vos_task_create(spin_task1, NULL, "spin_task1", 10, 4*1024);
	if (0 == g_tskhdl_spin1) {
		DBG_DUMP("create sem_task1 failed\r\n");
		return -1;
	}
	vos_task_resume(g_tskhdl_spin1);

	g_tskhdl_spin2 = vos_task_create(spin_task2, NULL, "spin_task2", 10, 4*1024);
	if (0 == g_tskhdl_spin2) {
		DBG_DUMP("create sem_task2 failed\r\n");
		return -1;
	}
	vos_task_resume(g_tskhdl_spin2);

	return 0;
}

int test_vos(int argc, char** argv)
{
	print_dbg_msg();

#ifdef __LINUX
	if (0 != dev_test()) {
		DBG_ERR("dev_test failed\r\n");
	}
#endif

#ifndef __LINUX_USER
	if (0 != mem_cma_test()) {
		DBG_ERR("mem_cma_test failed\r\n");
	}
#endif

	if (0 != spinlock_test()) {
		DBG_ERR("spinlock_test failed\r\n");
	}

	if (0 != sem_test()) {
		DBG_ERR("sem_test failed\r\n");
	}

	if (0 != flag_test()) {
		DBG_ERR("flag_test failed\r\n");
	}

	if (0 != file_test()) {
		DBG_ERR("file_test failed\r\n");
	}

	if (0 != sscanf_s_test("/mnt/sd 123")) {
		DBG_ERR("sscanf_s_test failed\r\n");
	}

#ifndef __LINUX
	if (0 != mbx_test()) {
		DBG_ERR("mbx_test failed\r\n");
	}
#endif

	return 0;
}
