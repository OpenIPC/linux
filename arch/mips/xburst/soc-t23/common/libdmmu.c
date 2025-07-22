


#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include <linux/mempolicy.h>
#include <linux/mm_types.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/current.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/io.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <jz_proc.h>

#include <mach/libdmmu.h>

#define MAP_COUNT		0x10
#define MAP_CONUT_MASK		0xff0

#define DMMU_PTE_VLD 		0x01
#define DMMU_PMD_VLD 		0x01

#define KSEG0_LOW_LIMIT		0x80000000
#define KSEG1_HEIGH_LIMIT	0xC0000000

LIST_HEAD(handle_list);
static unsigned long reserved_page = 0;
static unsigned long reserved_pte = 0;

struct pmd_node {
	unsigned int count;
	unsigned long index;
	unsigned long page;
	struct list_head list;
};

struct map_node {
	struct device *dev;
	unsigned long start;
	unsigned long len;
	struct list_head list;
};

struct dmmu_handle {
	pid_t tgid;
	unsigned long pdg;
	struct mutex lock;
	struct list_head list;
	struct list_head pmd_list;
	struct list_head map_list;
};

static struct map_node *check_map(struct dmmu_handle *h,unsigned long vaddr,unsigned long len)
{
	struct list_head *pos, *next;
	struct map_node *n;
	list_for_each_safe(pos, next, &h->map_list) {
		n = list_entry(pos, struct map_node, list);
		if(vaddr == n->start && len == n->len)
			return n;
	}
	return NULL;
}

static void handle_add_map(struct device *dev,struct dmmu_handle *h,unsigned long vaddr,unsigned long len)
{
	struct map_node *n = kmalloc(sizeof(*n),GFP_KERNEL);
	n->dev = dev;
	n->start = vaddr;
	n->len = len;
	INIT_LIST_HEAD(&n->list);
	list_add(&n->list,&h->map_list);
}

static unsigned int get_pfn(unsigned int vaddr)
{
	pgd_t *pgdir;
	pmd_t *pmdir;
	pte_t *pte;
	pgdir = pgd_offset(current->mm, vaddr);
	if(pgd_none(*pgdir) || pgd_bad(*pgdir))
		return 0;
	pmdir = pmd_offset((pud_t *)pgdir, vaddr);
	if(pmd_none(*pmdir) || pmd_bad(*pmdir))
		return 0;
	pte = pte_offset(pmdir,vaddr);
	if (pte_present(*pte)) {
		return pte_pfn(*pte) << PAGE_SHIFT;
	}
	return 0;
}

static unsigned long dmmu_v2pfn(unsigned long vaddr)
{
	if(vaddr < KSEG0_LOW_LIMIT)
		return get_pfn(vaddr);

	if(vaddr >= KSEG0_LOW_LIMIT && vaddr < KSEG1_HEIGH_LIMIT)
		return virt_to_phys((void *)vaddr);

	panic("dmmu_v2pfn error!");
	return 0;
}

static unsigned long unmap_node(struct pmd_node *n,unsigned long vaddr,unsigned long end,int check)
{
	unsigned int *pte = (unsigned int *)n->page;
	int index = ((vaddr & 0x3ff000) >> 12);
	int free = !check || (--n->count == 0);
	if(free) {
		__free_page((void *)n->page);
		list_del(&n->list);
		kfree(n);
		return vaddr+(1024-index)*4096;
	}

	while(index < 1024 && vaddr < end) {
		if(pte[index] & MAP_CONUT_MASK)
			pte[index] -= MAP_COUNT;
		else
			pte[index] = reserved_pte;
		index++;
		vaddr += 4096;
	}

	return vaddr;
}

static unsigned long map_node(struct pmd_node *n,unsigned int vaddr,unsigned int end)
{
	unsigned int *pte = (unsigned int *)n->page;
	int index = ((vaddr & 0x3ff000) >> 12);

	while(index < 1024 && vaddr < end) {
		if(pte[index] == reserved_pte) {
			pte[index] = dmmu_v2pfn(vaddr) | DMMU_PTE_VLD;
		} else {
			pte[index] += MAP_COUNT;
		}
		index++;
		vaddr += 4096;
	}
	n->count++;
	return vaddr;
}

static struct pmd_node *find_node(struct dmmu_handle *h,unsigned int vaddr)
{
	struct list_head *pos, *next;
	struct pmd_node *n;

	list_for_each_safe(pos, next, &h->pmd_list) {
		n = list_entry(pos, struct pmd_node, list);
		if(n->index == (vaddr & 0xffc00000))
			return n;
	}
	return NULL;
}

static struct pmd_node *add_node(struct dmmu_handle *h,unsigned int vaddr)
{
	int i;
	unsigned long *pte;
	unsigned long *pgd = (unsigned long *)h->pdg;
	struct pmd_node *n = kmalloc(sizeof(*n),GFP_KERNEL);
	INIT_LIST_HEAD(&n->list);
	n->count = 0;
	n->index = vaddr & 0xffc00000;
	n->page = __get_free_page(GFP_KERNEL);
	SetPageReserved(virt_to_page((void *)n->page));

	pte = (unsigned long *)n->page;
	for(i=0;i<1024;i++)
		pte[i] = reserved_pte;

	list_add(&n->list, &h->pmd_list);

	pgd[vaddr>>22] = dmmu_v2pfn(n->page) | DMMU_PMD_VLD;
	return n;
}

static struct dmmu_handle *find_handle(void)
{
	struct list_head *pos, *next;
	struct dmmu_handle *h;

	list_for_each_safe(pos, next, &handle_list) {
		h = list_entry(pos, struct dmmu_handle, list);
		if(h->tgid == current->tgid)
			return h;
	}
	return NULL;
}

static struct dmmu_handle *create_handle(void)
{
	struct dmmu_handle *h;

	h = kmalloc(sizeof(struct dmmu_handle),GFP_KERNEL);
	if(!h)
		return NULL;

	h->tgid = current->tgid;
	h->pdg = __get_free_page(GFP_KERNEL);
	SetPageReserved(virt_to_page((void *)h->pdg));

	if(reserved_pte == 0) {
		reserved_page = __get_free_page(GFP_KERNEL);
		SetPageReserved(virt_to_page((void *)reserved_page));
		reserved_pte = dmmu_v2pfn(reserved_page) | DMMU_PTE_VLD;
	}

	if(!h->pdg) {
		kfree(h);
		return NULL;
	}

	INIT_LIST_HEAD(&h->list);
	INIT_LIST_HEAD(&h->pmd_list);
	INIT_LIST_HEAD(&h->map_list);
	list_add(&h->list, &handle_list);

	mutex_init(&h->lock);
	return h;
}

static int dmmu_make_present(unsigned long addr,unsigned long end)
{
	int ret, len, write;
	struct vm_area_struct * vma;
	unsigned long vm_page_prot;

	vma = find_vma(current->mm, addr);
	if (!vma) {
		printk("dmmu_make_present error. addr=%lx len=%lx\n",addr,end-addr);
		return -1;
	}

	if(vma->vm_flags & VM_PFNMAP)
		return 0;

	write = (vma->vm_flags & VM_WRITE) != 0;
	BUG_ON(addr >= end);
	BUG_ON(end > vma->vm_end);

	vm_page_prot = pgprot_val(vma->vm_page_prot);
	vma->vm_page_prot = __pgprot(vm_page_prot | _PAGE_VALID| _PAGE_ACCESSED | _PAGE_PRESENT);

	len = DIV_ROUND_UP(end, PAGE_SIZE) - addr/PAGE_SIZE;
	ret = get_user_pages(current, current->mm, addr,
			len, write, 0, NULL, NULL);
	vma->vm_page_prot = __pgprot(vm_page_prot);
	if (ret < 0) {
		printk("dmmu_make_present get_user_pages error(%d). addr=%lx len=%lx\n",0-ret,addr,end-addr);
		return ret;
	}
	return ret == len ? 0 : -1;
}

static void dmmu_cache_wback(struct dmmu_handle *h)
{
	struct list_head *pos, *next;
	struct pmd_node *n;

	dma_cache_wback(h->pdg,PAGE_SIZE);

	list_for_each_safe(pos, next, &h->pmd_list) {
		n = list_entry(pos, struct pmd_node, list);
		dma_cache_wback(n->page,PAGE_SIZE);
	}
}

#ifdef DEBUG
static void dmmu_dump_handle(struct seq_file *m, void *v, struct dmmu_handle *h);
#endif
unsigned long dmmu_map(struct device *dev,unsigned long vaddr,unsigned long len)
{
	int end = vaddr + len;
	struct dmmu_handle *h;
	struct pmd_node *node;

	h = find_handle();
	if(!h)
		h = create_handle();
	if(!h)
		return 0;

	mutex_lock(&h->lock);
#ifdef DEBUG
	printk("(pid %d)dmmu_map %lx %lx================================================\n",h->tgid,vaddr,len);
#endif
	if(check_map(h,vaddr,len))
	{
		mutex_unlock(&h->lock);
		return dmmu_v2pfn(h->pdg);
	}

	if(dmmu_make_present(vaddr,vaddr+len))
	{
		mutex_unlock(&h->lock);
		return 0;
	}

	handle_add_map(dev,h,vaddr,len);

	while(vaddr < end) {
		node = find_node(h,vaddr);
		if(!node) {
			node = add_node(h,vaddr);
		}

		vaddr = map_node(node,vaddr,end);
	}

	dmmu_cache_wback(h);
#ifdef DEBUG
	dmmu_dump_handle(NULL,NULL,h);
#endif
	mutex_unlock(&h->lock);

	return dmmu_v2pfn(h->pdg);
}

int dmmu_unmap(struct device *dev,unsigned long vaddr, int len)
{
	unsigned long end = vaddr + len;
	struct dmmu_handle *h;
	struct pmd_node *node;
	struct map_node *n;

	h = find_handle();
	if(!h)
		return 0;

	mutex_lock(&h->lock);
#ifdef DEBUG
	printk("dmmu_unmap %lx %x**********************************************\n",vaddr,len);
#endif
	n = check_map(h,vaddr,len);
	if(!n) {
		mutex_unlock(&h->lock);
		return -EAGAIN;
	}
	if(n->dev != dev) {
		mutex_unlock(&h->lock);
		return -EAGAIN;
	}

	list_del(&n->list);
	kfree(n);

	while(vaddr < end) {
		node = find_node(h,vaddr);
		if(node)
			vaddr = unmap_node(node,vaddr,end,1);
	}

	if(list_empty(&h->pmd_list) && list_empty(&h->map_list)) {
		list_del(&h->list);
		__free_page((void *)h->pdg);
		if(list_empty(&handle_list)) {
			__free_page((void *)reserved_page);
			reserved_page = 0;
			reserved_pte = 0;
		}
		mutex_unlock(&h->lock);
		kfree(h);
		return 0;
	}

	if(list_empty(&handle_list)) {
		__free_page((void *)reserved_page);
		reserved_page = 0;
		reserved_pte = 0;
	}

	mutex_unlock(&h->lock);
	return 0;
}

int dmmu_free_all(struct device *dev)
{
	struct dmmu_handle *h;
	struct pmd_node *node;
	struct map_node *cn;
	struct list_head *pos, *next;

	h = find_handle();
	if(!h)
		return 0;

	mutex_lock(&h->lock);
	list_for_each_safe(pos, next, &h->pmd_list) {
		node = list_entry(pos, struct pmd_node, list);
		unmap_node(node,0,0,0);
	}

	list_for_each_safe(pos, next, &h->map_list) {
		cn = list_entry(pos, struct map_node, list);
		list_del(&cn->list);
		kfree(cn);
	}

	list_del(&h->list);
	__free_page((void *)h->pdg);
	kfree(h);

	if(list_empty(&handle_list)) {
		__free_page((void *)reserved_page);
		reserved_page = 0;
		reserved_pte = 0;
	}

	mutex_unlock(&h->lock);
	return 0;
}

int dmmu_unmap_all(struct device *dev)
{
	struct dmmu_handle *h;
	struct map_node *cn;
	struct list_head *pos, *next;

#ifdef DEBUG
	printk("dmmu_unmap_all\n");
#endif
	h = find_handle();
	if(!h)
		return 0;

	list_for_each_safe(pos, next, &h->map_list) {
		cn = list_entry(pos, struct map_node, list);
		if(dev == cn->dev)
			dmmu_unmap(dev,cn->start,cn->len);
	}


	if(list_empty(&h->map_list))
		dmmu_free_all(dev);
	return 0;
}

void dmmu_dump_vaddr(unsigned long vaddr)
{
	struct dmmu_handle *h;
	struct pmd_node *node;

	unsigned long *pmd,*pte;
	h = find_handle();
	if(!h) {
		printk("dmmu_dump_vaddr %08lx error - h not found!\n",vaddr);
		return;
	}

	node = find_node(h,vaddr);
	if(!node) {
		printk("dmmu_dump_vaddr %08lx error - node not found!\n",vaddr);
		return;
	}

	pmd = (unsigned long *)h->pdg;
	pte = (unsigned long *)node->page;

	printk("pmd base = %p; pte base = %p\n",pmd,pte);
	printk("pmd = %08lx; pte = %08lx\n",pmd[vaddr>>22],pte[(vaddr&0x3ff000)>>12]);
}

#ifdef DEBUG
static void dmmu_dump_handle(struct seq_file *m, void *v, struct dmmu_handle *h)
{
	struct list_head *pos, *next;
	struct map_node *n;

	list_for_each_safe(pos, next, &h->map_list) {
		n = list_entry(pos, struct map_node, list);
		{
			int i = 0;
			int vaddr = n->start;
			struct pmd_node *pn = find_node(h,vaddr);
			unsigned int *pte = (unsigned int *)pn->page;

			while(vaddr < (n->start + n->len)) {
				if(i++%8 == 0)
					printk("\nvaddr %08x : ",vaddr & 0xfffff000);
				printk("%08x ",pte[(vaddr & 0x3ff000)>>12]);
				vaddr += 4096;
			}
			printk("\n\n");
		}
	}
}
#endif
#if 0
static int dmmu_proc_show(struct seq_file *m, void *v)
{
	struct list_head *pos, *next;
	struct dmmu_handle *h;
	volatile unsigned long flags;
	local_irq_save(flags);
		list_for_each_safe(pos, next, &handle_list) {
			h = list_entry(pos, struct dmmu_handle, list);
			dmmu_dump_handle(m, v, h);
		}
	local_irq_restore(flags);
	return 0;
}

static int dmmu_open(struct inode *inode, struct file *file)
{
	return single_open(file, dmmu_proc_show, PDE_DATA(inode));
}

static const struct file_operations dmmus_proc_fops ={
	.read = seq_read,
	.open = dmmu_open,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init init_dmmu_proc(void)
{
	struct proc_dir_entry *p;
	p = jz_proc_mkdir("dmmu");
	if (!p) {
		pr_warning("create_proc_entry for common dmmu failed.\n");
		return -ENODEV;
	}
	proc_create("dmmus", 0600,p,&dmmus_proc_fops);

	return 0;
}

module_init(init_dmmu_proc);
#endif
