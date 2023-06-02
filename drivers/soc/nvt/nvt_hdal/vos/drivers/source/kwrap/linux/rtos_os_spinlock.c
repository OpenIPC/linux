/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/spinlock.h>

#define __MODULE__    rtos_spinlock
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/spinlock.h>


#define VOS_SPINLOCK_DEBUG              0 //0: disable, 1: enable
#define VOS_SPINLOCK_DEBUG_MAX_CORE     2
#define VOS_SPINLOCK_DEBUG_LIST_NUM     64
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define VOS_SPINLOCK_TAG_INITED MAKEFOURCC('V', 'S', 'P', 'N')

#ifndef sizeof_field
#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))
#endif

STATIC_ASSERT(sizeof_field(vk_spinlock_t, buf) >= sizeof(spinlock_t));
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_spinlock_debug_level = NVT_DBG_WRN;
static DEFINE_SPINLOCK(vos_inter_lock);

module_param_named(rtos_spinlock_debug_level, rtos_spinlock_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rtos_spinlock_debug_level, "Debug message level");

#if VOS_SPINLOCK_DEBUG
typedef struct {
	unsigned int next_idx;
	unsigned long caller_ra[VOS_SPINLOCK_DEBUG_LIST_NUM];
} VOS_SPINLOCK_LIST;

static VOS_SPINLOCK_LIST g_vos_lock_list[VOS_SPINLOCK_DEBUG_MAX_CORE] = {0};
static VOS_SPINLOCK_LIST g_vos_unlock_list[VOS_SPINLOCK_DEBUG_MAX_CORE] = {0};
#endif

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
#if VOS_SPINLOCK_DEBUG
static inline void _vk_spin_add_lock_list(void)
{
	unsigned long inter_flags = 0;
	unsigned int cpu_id;
	VOS_SPINLOCK_LIST *p_list;
	static int print_once = 0;

	spin_lock_irqsave(&vos_inter_lock, inter_flags);

	if (!print_once) {
		print_once = 1;
		DBG_DUMP("[VOS_SPIN] max_core %d list_num %d\r\n", VOS_SPINLOCK_DEBUG_MAX_CORE, VOS_SPINLOCK_DEBUG_LIST_NUM);
		for (cpu_id = 0; cpu_id < VOS_SPINLOCK_DEBUG_MAX_CORE; cpu_id++) {
			DBG_DUMP("[VOS_SPIN] cpu%d loc next_idx = 0x%lX\r\n", cpu_id, (unsigned long)&g_vos_lock_list[cpu_id].next_idx);
			DBG_DUMP("[VOS_SPIN] cpu%d loc caller_ra[0] = 0x%lX\r\n", cpu_id, (unsigned long)&g_vos_lock_list[cpu_id].caller_ra[0]);
			DBG_DUMP("[VOS_SPIN] cpu%d unl next_idx = 0x%lX\r\n", cpu_id, (unsigned long)&g_vos_unlock_list[cpu_id].next_idx);
			DBG_DUMP("[VOS_SPIN] cpu%d unl caller_ra[0] = 0x%lX\r\n", cpu_id, (unsigned long)&g_vos_unlock_list[cpu_id].caller_ra[0]);
		}
	}

	cpu_id = smp_processor_id();
	if (cpu_id > VOS_SPINLOCK_DEBUG_MAX_CORE - 1) {
		DBG_ERR("Exceed max core 0x%x\r\n", VOS_SPINLOCK_DEBUG_MAX_CORE);
		spin_unlock_irqrestore(&vos_inter_lock, inter_flags);
		return;
	}

	p_list = &g_vos_lock_list[cpu_id];
	p_list->caller_ra[p_list->next_idx] = (unsigned long)__builtin_return_address(0);

	p_list->next_idx++;
	if (p_list->next_idx >= VOS_SPINLOCK_DEBUG_LIST_NUM) {
		p_list->next_idx = 0;
	}

	spin_unlock_irqrestore(&vos_inter_lock, inter_flags);
}

static inline void _vk_spin_add_unlock_list(void)
{
	unsigned long inter_flags = 0;
	unsigned int cpu_id;
	VOS_SPINLOCK_LIST *p_list;

	spin_lock_irqsave(&vos_inter_lock, inter_flags);

	cpu_id = smp_processor_id();
	if (cpu_id > VOS_SPINLOCK_DEBUG_MAX_CORE - 1) {
		DBG_ERR("Exceed max core 0x%x\r\n", VOS_SPINLOCK_DEBUG_MAX_CORE);
		spin_unlock_irqrestore(&vos_inter_lock, inter_flags);
		return;
	}

	p_list = &g_vos_unlock_list[cpu_id];
	p_list->caller_ra[p_list->next_idx] = (unsigned long)__builtin_return_address(0);

	p_list->next_idx++;
	if (p_list->next_idx >= VOS_SPINLOCK_DEBUG_LIST_NUM) {
		p_list->next_idx = 0;
	}

	spin_unlock_irqrestore(&vos_inter_lock, inter_flags);
}
#endif //#if SPIN_LOCK_DEBUG

void vk_spin_dump_list(void)
{
#if !VOS_SPINLOCK_DEBUG
	DBG_DUMP("VOS_SPINLOCK_DEBUG is disable\r\n");
	return;
#else
	unsigned long inter_flags = 0;
	unsigned int cpu_id;
	unsigned int list_idx;

	spin_lock_irqsave(&vos_inter_lock, inter_flags);

	for (cpu_id = 0; cpu_id < VOS_SPINLOCK_DEBUG_MAX_CORE; cpu_id++) {
		DBG_DUMP("cpu%d loc next_idx 0x%X\r\n", cpu_id, g_vos_lock_list[cpu_id].next_idx);
		for (list_idx = 0; list_idx < VOS_SPINLOCK_DEBUG_LIST_NUM; list_idx++) {
			DBG_DUMP("cpu%d[%03d] loc_ra 0x%lX\r\n", cpu_id, list_idx, 
				g_vos_lock_list[cpu_id].caller_ra[list_idx]);
		}

		DBG_DUMP("cpu%d unl next_idx 0x%X\r\n", cpu_id, g_vos_unlock_list[cpu_id].next_idx);
		for (list_idx = 0; list_idx < VOS_SPINLOCK_DEBUG_LIST_NUM; list_idx++) {
			DBG_DUMP("cpu%d[%03d] unl_ra 0x%lX\r\n", cpu_id, list_idx, 
				g_vos_unlock_list[cpu_id].caller_ra[list_idx]);
		}
	}

	spin_unlock_irqrestore(&vos_inter_lock, inter_flags);
#endif
}
EXPORT_SYMBOL(vk_spin_dump_list);

void vk_spin_lock_init(vk_spinlock_t *p_voslock)
{
	unsigned long inter_flags = 0;
	spinlock_t *p_spinlock = (spinlock_t *)p_voslock->buf;

	spin_lock_irqsave(&vos_inter_lock, inter_flags);

	//always init spinlock without checking init_tag to prevent dirty data
	//coverity[side_effect_free]: spin_lock_init is kernel API, do not change
	spin_lock_init(p_spinlock);
	p_voslock->init_tag = VOS_SPINLOCK_TAG_INITED;

	spin_unlock_irqrestore(&vos_inter_lock, inter_flags);
}

static void _vk_spin_lock_init_check_tag(vk_spinlock_t *p_voslock)
{
	unsigned long inter_flags = 0;

	spin_lock_irqsave(&vos_inter_lock, inter_flags);

	//after inter lock, check the tag again to make sure not inited yet
	if (VOS_SPINLOCK_TAG_INITED != p_voslock->init_tag) {
		spinlock_t *p_spinlock = (spinlock_t *)p_voslock->buf;
		// coverity[side_effect_free]: spin_lock_init is kernel API, do not change
		spin_lock_init(p_spinlock);
		p_voslock->init_tag = VOS_SPINLOCK_TAG_INITED;
	}

	spin_unlock_irqrestore(&vos_inter_lock, inter_flags);
}

unsigned long _vk_raw_spin_lock_irqsave(vk_spinlock_t *p_voslock)
{
	spinlock_t *p_spinlock = (spinlock_t *)p_voslock->buf;
	unsigned long flags = 0;

	if (VOS_SPINLOCK_TAG_INITED != p_voslock->init_tag) {
		_vk_spin_lock_init_check_tag(p_voslock);
	}

#if VOS_SPINLOCK_DEBUG
	_vk_spin_add_lock_list();
#endif

	spin_lock_irqsave(p_spinlock, flags);

	return flags;
}

void vk_spin_unlock_irqrestore(vk_spinlock_t *p_voslock, unsigned long flags)
{
	spinlock_t *p_spinlock = (spinlock_t *)p_voslock->buf;

	if (VOS_SPINLOCK_TAG_INITED != p_voslock->init_tag) {
		DBG_ERR("lock not inited\r\n");
		return;
	}

#if VOS_SPINLOCK_DEBUG
	_vk_spin_add_unlock_list();
#endif

	spin_unlock_irqrestore(p_spinlock, flags);
}

EXPORT_SYMBOL(vk_spin_lock_init);
EXPORT_SYMBOL(_vk_raw_spin_lock_irqsave);
EXPORT_SYMBOL(vk_spin_unlock_irqrestore);
