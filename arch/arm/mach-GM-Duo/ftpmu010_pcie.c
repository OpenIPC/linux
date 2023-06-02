#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/io.h>
#include <mach/platform/platform_io.h>
#include <linux/platform_device.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <mach/ftpmu010.h>
#include <mach/ftpmu010_pcie.h>
#include <mach/platform/board.h>

#define  REG_ARRAY_SZ   30  //Max value for register array


/* Local Variables
 */
static u32  fd = 1;
static struct proc_dir_entry *pmu_proc_root = NULL;
static struct proc_dir_entry *attribute_proc = NULL;
static struct proc_dir_entry *regInfo_proc = NULL;
static ftpmu010_pcie_gate_clk_t  *pmu_clkgate_tbl = NULL;
static int (*pmu_ctrl_handler)(u32 cmd, u32 data1, u32 data2) = NULL;

/* Local Functions
 */
static int ftpmu010_pcie_proc_init(void);

/* MACROs
 */
#define ATTR_LIST               ftpmu10_pcie.attr_list
#define REGINFO_LIST            ftpmu10_pcie.reginfo_list
#define LIST_LOCK               spin_lock_irqsave(&ftpmu10_pcie.spinlock, flags)
#define LIST_UNLOCK             spin_unlock_irqrestore(&ftpmu10_pcie.spinlock, flags)
#define REGINFO_OFFSET(x,y)     (x)->pmuReg_info.pRegArray[(y)].reg_off
#define REGINFO_BITSMASK(x,y)   (x)->pmuReg_info.pRegArray[(y)].bits_mask
#define REGINFO_LOCKBITS(x,y)   (x)->pmuReg_info.pRegArray[(y)].lock_bits
#define REGINFO_NAME(x)         (x)->pmuReg_info.name
#define REGINFO_REGCNT(x)       (x)->pmuReg_info.num
#define REGINFO_CLKSRC          (x)->pmuReg_info.clock_src

typedef struct {
    attrPcieInfo_t      attr_info;
    struct list_head    list;
} attrPcieInfo_node_t;

typedef struct {
    u32                 fd; /* unique number */
    pmuPcieRegInfo_t    pmuReg_info;
    struct list_head    list;
    wait_queue_head_t   wait_queue;     /* wait queue */
    int	                waiting;
    int                 ref_cnt;    
} pmuPcieRegInfo_node_t;

/* main structure
 */
static struct ftpmu10_pcie_s
{
    void __iomem        *base;
    spinlock_t          spinlock;
    /* attribute list */
    struct list_head    attr_list;
    /* register list */
    struct list_head    reginfo_list;
} ftpmu10_pcie;

/* Register a clok node.
 * return value: 0 for success, < 0 for fail.
 */
int ftpmu010_pcie_register_attr(attrPcieInfo_t *attr)
{
    unsigned long       flags;
    attrPcieInfo_node_t *node;
    int                 ret = 0;

    /* lock the database */
    LIST_LOCK;

    /* walk through whole attribute list */
    list_for_each_entry(node, &ATTR_LIST, list)
    {
        //printk("PMU: %s \n", node->attr_info.name);

        if (node->attr_info.attr_type == attr->attr_type)
        {
            ret = -1;
            break;
        }
    }
    /* unlock */
    LIST_UNLOCK;

    if (ret < 0)
        return ret;

    ret = -1;
    node = kzalloc(sizeof(attrPcieInfo_node_t), GFP_ATOMIC);
    if (node)
    {
        memcpy(&node->attr_info, attr, sizeof(attrPcieInfo_t));
        INIT_LIST_HEAD(&node->list);
        list_add_tail(&node->list, &ATTR_LIST);
        try_module_get(THIS_MODULE);
        ret = 0;
    }

    return ret;
}

/* DeRegister a clok node.
 * return value: 0 for success, < 0 for fail.
 */
int ftpmu010_pcie_deregister_attr(attrPcieInfo_t *attr)
{
    unsigned long flags;
    attrPcieInfo_node_t  *node, *ne;
    int     ret = -1;

    /* lock */
    LIST_LOCK;

    /* walk through whole attribute list */
    list_for_each_entry_safe(node, ne, &ATTR_LIST, list)
    {
        if (node->attr_info.attr_type == attr->attr_type)
        {
            list_del_init(&node->list);
            kfree(node);
            module_put(THIS_MODULE);
            ret = 0;
            break;
        }
    }
    /* unlock */
    LIST_UNLOCK;

    return ret;
}

/* get the content of the attribute
 * return value: 0 for fail, > 0 for success
 */
unsigned int ftpmu010_pcie_get_attr(ATTR_PCIE_TYPE_T  attr_type)
{
    attrPcieInfo_node_t  *node;
    unsigned int     value = -1;

    /* walk through whole attribute list */
    list_for_each_entry(node, &ATTR_LIST, list)
    {
        if (node->attr_info.attr_type == attr_type)
        {
            value = node->attr_info.value;
            break;
        }
    }

    return value;
}

/* register/de-register the register table
 * return value:
 *  give an unique fd if return value >= 0, otherwise < 0 if fail.
 */
int ftpmu010_pcie_register_reg(pmuPcieRegInfo_t *info)
{
    unsigned long   flags;
    pmuPcieRegInfo_node_t   *node, *new_node;
    int     i, j, ret = -1;
    u32     reg_off;

    /* sanity check */
    if (unlikely(!strlen(info->name)))
        return -1;

    /* check if the register offset is duplicated */
    for (i = 0; i < info->num; i ++)
    {
        reg_off = info->pRegArray[i].reg_off;

        for (j = 0; j < info->num; j ++)
        {
            if (i == j)
                continue;
            if (reg_off == info->pRegArray[j].reg_off)
            {
                printk("PMU: reg_offset 0x%x is duplicated! \n", reg_off);
                return -1;
            }
        }
    }

    /* self-test, bits_mask must cover lock_bits and init_val */
    for (i = 0; i < info->num; i ++)
    {
        if ((~info->pRegArray[i].bits_mask) & info->pRegArray[i].lock_bits)
        {
            printk("PMU: %s wrong lock_bits! \n", info->name);
            return -1;
        }
        if ((~info->pRegArray[i].bits_mask) & info->pRegArray[i].init_val)
        {
            printk("PMU: %s wrong init_val! \n", info->name);
            return -1;
        }
        if ((~info->pRegArray[i].bits_mask) & info->pRegArray[i].init_mask)
        {
            printk("PMU: %s wrong init_mask! \n", info->name);
            return -1;
        }
        if ((~info->pRegArray[i].init_mask) & info->pRegArray[i].init_val)
        {
            printk("PMU: %s wrong init_val or init_mask! \n", info->name);
            return -1;
        }
    }

    if (info->clock_src != ATTR_TYPE_PCIE_NONE)
    {
        if (ftpmu010_get_attr(info->clock_src) < 0)
        {
            printk("PMU: %s registers non-existence clock source! \n", info->name);
            return -1;
        }
    }

    /* lock */
    LIST_LOCK;

    /* 1. check if duplicate registeration
     */
    list_for_each_entry(node, &REGINFO_LIST, list)
    {
        if (!strcmp(REGINFO_NAME(node), info->name))
        {
            /* allow the same node is multiple registered */
            if ((info->num == REGINFO_REGCNT(node)) && !memcmp(info->pRegArray, node->pmuReg_info.pRegArray, info->num * sizeof(pmuReg_t)))
            {
                node->ref_cnt ++;
                /* unlock */
                LIST_UNLOCK;
                return node->fd;
            }            
            printk("PMU: %s was registed already! \n", info->name);
            goto exit;
        }
    }

    /* 2. check if the lockbits is overlap.
     */
    for (i = 0; i < info->num; i ++)
    {
        /* check the registers in each node */
        list_for_each_entry(node, &REGINFO_LIST, list)
        {
            for (j = 0; j < REGINFO_REGCNT(node); j ++)
            {
                if (info->pRegArray[i].reg_off != REGINFO_OFFSET(node, j))
                    continue;

                if (info->pRegArray[i].lock_bits & REGINFO_LOCKBITS(node, j))
                {
                    printk("PMU: lock_bits of %s is overlapped with %s in offset 0x%x! \n",
                            info->name, REGINFO_NAME(node), info->pRegArray[i].reg_off);
                    goto exit;
                }
            } /* loop j */
        }
    } /* loop i */

    /* 3. sanity check is ok, create new node and chan it to the list
     */
    new_node = kzalloc(sizeof(pmuPcieRegInfo_node_t), GFP_ATOMIC);
    if (unlikely(!new_node))
    {
        ret = -ENOMEM;
        goto exit;
    }
    new_node->fd = fd++;
    INIT_LIST_HEAD(&new_node->list);
    memcpy(&new_node->pmuReg_info, info, sizeof(pmuPcieRegInfo_t));
    if (info->num >= REG_ARRAY_SZ)
        panic("%s, the array size %d is too small! \n", __func__, REG_ARRAY_SZ);
    new_node->pmuReg_info.pRegArray = kzalloc(REG_ARRAY_SZ * sizeof(pmuPcieReg_t), GFP_ATOMIC);
    if (unlikely(!new_node->pmuReg_info.pRegArray))
    {
        kfree(new_node);
        ret = -ENOMEM;
        goto exit;
    }

    /* init waitQ */
    init_waitqueue_head(&new_node->wait_queue);

    /* copy register array body */
    if (info->num)
        memcpy(new_node->pmuReg_info.pRegArray, info->pRegArray, info->num * sizeof(pmuPcieReg_t));

    list_add_tail(&new_node->list, &REGINFO_LIST);

    /* unlock */
    LIST_UNLOCK;

    ret = new_node->fd;
    try_module_get(THIS_MODULE);

    /* update to hardware */
    for (i = 0; i < info->num; i ++)
        ftpmu010_pcie_write_reg(new_node->fd, info->pRegArray[i].reg_off, info->pRegArray[i].init_val, info->pRegArray[i].init_mask);

    return ret;

exit:
    /* unlock */
    LIST_UNLOCK;
    return ret;
}

int ftpmu010_pcie_deregister_reg(int fd)
{
    unsigned long flags;
    pmuPcieRegInfo_node_t   *node, *ne;
    int     ret = -1;

    /* lock */
    LIST_LOCK;

    list_for_each_entry_safe(node, ne, &REGINFO_LIST, list)
    {
        if (node->fd == fd)
        {
            if (-- node->ref_cnt) {
                ret = 0;
                goto exit;
            }
            
            //printk("PMU: %s is deregistered. \n", node->pmuReg_info.name);
            list_del_init(&node->list);
            if (node->pmuReg_info.pRegArray)
                kfree(node->pmuReg_info.pRegArray);
            kfree(node);
            module_put(THIS_MODULE);
            ret = 0;
            break;
        }
    }

exit:
    /* unlock */
    LIST_UNLOCK;
    return ret;
}

/* PMU register read/write
 */
unsigned int ftpmu010_pcie_read_reg(unsigned int reg_off)
{
    return ioread32(ftpmu10_pcie.base + reg_off);
}

/* return value < 0 for fail */
int ftpmu010_pcie_write_reg(int fd, unsigned int reg_off, unsigned int val, unsigned int mask)
{
    pmuPcieRegInfo_node_t   *node;
    int                 i, ret = -1;

    /* sanity check */
    if (unlikely(val & (~mask))) {
        printk("%s: wrong mask:%#x or value:%#x in offset:%#x! \n", __func__, mask, val, reg_off);
        goto exit;
    }

    if (unlikely(!mask)) {
        ret = 0;
	    goto exit;	/* do nothing */
    }

    list_for_each_entry(node, &REGINFO_LIST, list)
    {
        if (node->fd == fd)
        {
            ret = -1;
            /* check if reg_off had been registered already */
            for (i = 0; i < REGINFO_REGCNT(node); i ++)
            {
                if (REGINFO_OFFSET(node, i) != reg_off)
                    continue;

                /* is mask within bits_mask ? */
                if (~REGINFO_BITSMASK(node, i) & mask)
                {
                    printk("PMU: %s writes with wrong mask 0x%x in off:%#x! \n",
                        REGINFO_NAME(node), mask, reg_off);
                    ret = -1;
                    goto exit;
                }
                ret = 0;
            }
        }
    }

    if (!ret)
    {
        u32     tmp;

        tmp = (ftpmu010_pcie_read_reg(reg_off) & (~mask));
        tmp |= (val & mask);
        iowrite32(tmp, ftpmu10_pcie.base + reg_off);
    }

exit:
    if (ret < 0) {
        dump_stack();
        panic("Configure PMU fail! \n");
    }

    return ret;
}

/* PMU init function
 * Input parameters: virtual address of PMU
 * Return: 0 for success, < 0 for fail
 */
int __init ftpmu010_pcie_init(void __iomem  *base, ftpmu010_pcie_gate_clk_t *tbl, void *pmu_handler)
{
    int     i = 0;
    ftpmu010_pcie_gate_clk_t *table = tbl;
    
    ftpmu10_pcie.base = base;
    spin_lock_init(&ftpmu10_pcie.spinlock);
    INIT_LIST_HEAD(&ftpmu10_pcie.attr_list);
    INIT_LIST_HEAD(&ftpmu10_pcie.reginfo_list);
    /* proc function */
    ftpmu010_pcie_proc_init();

    /* self test */
    while (table->midx != FTPMU_PCIE_NONE) {
        for (i = 0; i < table->num; i ++) {
            if (table->reg[i].bit_val & ~table->reg[i].bit_mask)
                panic("%s, error gating clock table in ofs 0x%x of midx:%d! \n", __func__,
                        table->reg[i].ofs, table->midx);
        }
        table ++;
    }

    pmu_clkgate_tbl = tbl;
    /* register the callback from pmu */
    pmu_ctrl_handler = pmu_handler;
    
    return 0;
}

/* ------------------------------------------------------------------------------------
 *  Proc function
 * ------------------------------------------------------------------------------------
 */

 /* Attribute info
  */
static int proc_read_attribute_info(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned long flags;
    attrPcieInfo_node_t *node;
    int     len = 0;

    /* lock the database */
    LIST_LOCK;

    /* walk through whole attribute list */
    list_for_each_entry(node, &ATTR_LIST, list)
    {
        len += sprintf(page+len, "Attribute name : %s \n", node->attr_info.name);
        len += sprintf(page+len, "Attribute type : %d \n", node->attr_info.attr_type);
        len += sprintf(page+len, "Attribute value: %d \n\n", node->attr_info.value);
    }
    /* unlock */
    LIST_UNLOCK;

    return len;
}

/* Register info
 */
static int proc_read_reginfo(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned long flags;
    pmuPcieRegInfo_node_t *node;
    int     i, len = 0;

    /* lock the database */
    LIST_LOCK;

    /* walk through whole attribute list */
    list_for_each_entry(node, &REGINFO_LIST, list)
    {
        len += sprintf(page+len, "name: %s, fd = %d, attribute type = %d \n", REGINFO_NAME(node), node->fd, node->pmuReg_info.clock_src);
        for (i = 0; i < REGINFO_REGCNT(node); i ++)
        {
            len += sprintf(page+len,"    reg_off: 0x%x, bits_mask: 0x%x, lock_bits: 0x%x\n",
                        REGINFO_OFFSET(node, i), REGINFO_BITSMASK(node, i), REGINFO_LOCKBITS(node, i));
        }
        len += sprintf(page+len, "\n");
    }
    /* unlock */
    LIST_UNLOCK;

    return len;
}

static int ftpmu010_pcie_proc_init(void)
{
    int     ret = 0;
    struct proc_dir_entry   *p;

	p = create_proc_entry("pmu_pcie", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
	if (p == NULL) {
		return -ENOMEM;
	}
	pmu_proc_root = p;

	/*
     * attribute
     */
	attribute_proc = create_proc_entry("attribute", S_IRUGO, pmu_proc_root);
	if (attribute_proc == NULL) {
		printk("PMU: Fail to create proc attribute!\n");
		remove_proc_entry(pmu_proc_root->name, NULL);
		ret = -EINVAL;
		goto end;
	}
	attribute_proc->read_proc = (read_proc_t *) proc_read_attribute_info;
	attribute_proc->write_proc = NULL;

    /*
     * regInfo
     */
    regInfo_proc = create_proc_entry("reginfo", S_IRUGO, pmu_proc_root);
	if (regInfo_proc == NULL) {
		printk("PMU: Fail to create proc regInfo!\n");
		remove_proc_entry(attribute_proc->name, pmu_proc_root);
		remove_proc_entry(pmu_proc_root->name, NULL);
		ret = -EINVAL;
		goto end;
	}
	regInfo_proc->read_proc = (read_proc_t *) proc_read_reginfo;
	regInfo_proc->write_proc = NULL;
end:
	return ret;
}

EXPORT_SYMBOL(ftpmu010_pcie_get_attr);
EXPORT_SYMBOL(ftpmu010_pcie_register_reg);
EXPORT_SYMBOL(ftpmu010_pcie_deregister_reg);
EXPORT_SYMBOL(ftpmu010_pcie_read_reg);
EXPORT_SYMBOL(ftpmu010_pcie_write_reg);
