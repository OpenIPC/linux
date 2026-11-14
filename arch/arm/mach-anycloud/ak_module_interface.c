#include <mach/ak_module_interface.h>

/***************** MACRO HELPER  Level 1 ***********************************/
/*
* AKMI_DEF_GET_MEM_FUNC(mmc_host,void *,private) 
*  ==>
*  void *ak_mi_mmc_host_get_private(struct mmc_host* ptr)    \
*  {   \
*   return ptr->private;    \
*  };
*
*/
#define AKMI_DEF_GET_MEM_FUNC(str, type, mem)   \
    type ak_mi_##str##_get_##mem(struct str* ptr)    \
{   \
    return ptr->mem;    \
};                          \
EXPORT_SYMBOL(ak_mi_##str##_get_##mem); 

#define AKMI_DEF_GET_MEM_PTR_FUNC(str, type, mem)   \
    type* ak_mi_##str##_get_##mem##_##ptr(struct str* ptr)    \
{   \
    return &ptr->mem;    \
};                          \
EXPORT_SYMBOL(ak_mi_##str##_get_##mem##_##ptr); 

#define AKMI_DEF_SET_MEM_FUNC(str, type, mem)   \
    void ak_mi_##str##_set_##mem(struct str* ptr, type val)    \
{   \
       ptr->mem = val;  \
};                          \
EXPORT_SYMBOL(ak_mi_##str##_set_##mem); 

/******************* MACRO HELPER  Level 2 ***********************************/
/*  Level 2  
 * 
 *  For example:
 *  AKMI_DEF_GET_MEM_FUNC_L2(platform_device, struct device_node*, dev.of_node,
 *          dev, of_node)
 *  ==>
 *  struct device_node* ak_mi_platform_device_get_dev_of_node(struct str* ptr) \
 *  {   \
 *   return ptr->dev.of_node;    \
 *  };     
 * 
 */

#define AKMI_DEF_GET_MEM_FUNC_L2(str, type, mem, n1,n2)   \
    type ak_mi_##str##_get_##n1##_##n2(struct str* ptr)    \
{   \
        return ptr->mem;    \
};                          \
EXPORT_SYMBOL(ak_mi_##str##_get_##n1##_##n2); 

#define AKMI_DEF_GET_MEM_PTR_FUNC_L2(str, type, mem, n1,n2)   \
    type* ak_mi_##str##_get_##n1##_##n2##_##ptr(struct str* ptr)    \
{   \
        return &ptr->mem;    \
};                          \
EXPORT_SYMBOL(ak_mi_##str##_get_##n1##_##n2##_##ptr); 


#define AKMI_DEF_SET_MEM_FUNC_L2(str, type, mem, n1, n2)   \
    void ak_mi_##str##_set_##n1##_##n2(struct str* ptr, type val)    \
{   \
       ptr->mem = val;  \
};                          \
EXPORT_SYMBOL(ak_mi_##str##_set_##n1##_##n2); 


/************************** MACRO HELPER  Level 3 ****************************/
#define AKMI_DEF_MEM_FUNC_L3(str, type, mem)   \
    AKMI_DEF_GET_MEM_FUNC_L3(str, type, mem)   \
    AKMI_DEF_SET_MEM_FUNC_L3(str, type, mem)   

#define AKMI_DEF_GET_MEM_FUNC_L3(str, type, mem, n1,n2,n3)   \
    type ak_mi_##str##_get_##n1##_##n2##_##n3(struct str* ptr)    \
{   \
        return ptr->mem;    \
};                          \
EXPORT_SYMBOL(ak_mi_##str##_get_##n1##_##n2##_##n3); 

#define AKMI_DEF_GET_MEM_PTR_FUNC_L3(str, type, mem, n1,n2,n3)   \
    type* ak_mi_##str##_get_##n1##_##n2##_##n3##_##ptr(struct str* ptr)    \
{   \
        return &ptr->mem;    \
};                          \
EXPORT_SYMBOL(ak_mi_##str##_get_##n1##_##n2##_##n3##_##ptr); 

#define AKMI_DEF_SET_MEM_FUNC_L3(str, type, mem, n1, n2, n3)   \
    void ak_mi_##str##_set_##n1##_##n2##_##n3(struct str* ptr, type val)    \
{   \
       ptr->mem = val;  \
};                          \
EXPORT_SYMBOL(ak_mi_##str##_set_##n1##_##n2##_##n3); 


/******************************* For SIZE TABLE **************************/
struct struct_size_talbe struct_size_talbe =
{
    AKMI_SIZE_INIT(iio_dev)  // struct iio_dev  --> int iio_dev;
    AKMI_SIZE_INIT(video_device)  // struct video_device --> int video_device;
};
EXPORT_SYMBOL(struct_size_talbe);


/*************************** For ak_mci.ko ***********************************/
AKMI_DEF_GET_MEM_FUNC(mmc_host,void *,private)
AKMI_DEF_GET_MEM_FUNC(file,void *,private_data)
AKMI_DEF_SET_MEM_FUNC(file,void *,private_data)
AKMI_DEF_GET_MEM_FUNC_L2(platform_device,void * ,dev.of_node, dev, of_node)
AKMI_DEF_GET_MEM_FUNC_L2(platform_device,void * ,dev.pins, dev, pins)

/************************** AKMI For ak_i2c.ko ***************************/
AKMI_DEF_GET_MEM_FUNC(i2c_adapter, const char *, name)
AKMI_DEF_SET_MEM_FUNC(i2c_adapter, int, nr)
AKMI_DEF_GET_MEM_FUNC(i2c_adapter, int, nr)

/*************************** AKMI For ak_motor.ko ****************************/
AKMI_DEF_GET_MEM_FUNC(inode, struct cdev *, i_cdev)

/**************************** AKMI For ak_saradc.ko **************************/
AKMI_DEF_SET_MEM_FUNC(device, struct device_node *, of_node)

/****************************** AKMI For ak_i2s.ko ****************************/
AKMI_DEF_GET_MEM_FUNC(device, struct device_node *, of_node)

/****************************** AKMI For ak_hcd.ko ****************************/
AKMI_DEF_GET_MEM_FUNC_L2(platform_device,u64,dev.coherent_dma_mask,dev, \
    coherent_dma_mask)
AKMI_DEF_SET_MEM_FUNC_L2(platform_device,u64,dev.coherent_dma_mask,dev, \
    coherent_dma_mask)
AKMI_DEF_GET_MEM_PTR_FUNC_L2(platform_device,u64,dev.coherent_dma_mask,dev, \
    coherent_dma_mask)
AKMI_DEF_GET_MEM_FUNC_L2(platform_device,u64*,dev.dma_mask,dev,dma_mask)
AKMI_DEF_SET_MEM_FUNC_L2(platform_device,u64*,dev.dma_mask,dev,dma_mask)

/****************************** AKMI For ak_udc.ko ****************************/
AKMI_DEF_GET_MEM_FUNC(platform_device,u32,num_resources)
AKMI_DEF_GET_MEM_FUNC(platform_device,struct resource *,resource)

