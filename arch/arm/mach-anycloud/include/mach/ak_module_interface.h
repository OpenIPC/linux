
#ifndef __MODULES_INTERFACE_H
#define __MODULES_INTERFACE_H

/*********************** Extern Get or Set Function HELPER ********************/
/* 
 * Level 1
 */
#define AKMI_EXTERN_GET_MEM_FUNC(str, type, mem)   \
    extern type ak_mi_##str##_get_##mem(struct str* ptr);
#define AKMI_EXTERN_GET_MEM_PTR_FUNC(str, type, mem)   \
    type* ak_mi_##str##_get_##mem##_##ptr(struct str* ptr) ;
#define AKMI_EXTERN_SET_MEM_FUNC(str, type, mem)   \
    extern void ak_mi_##str##_set_##mem(struct str* ptr, type val);
/*  
 * Level 2  
 */
#define AKMI_EXTERN_GET_MEM_FUNC_L2(str, type, mem, n1,n2)   \
    extern type ak_mi_##str##_get_##n1##_##n2(struct str* ptr);

#define AKMI_EXTERN_GET_MEM_PTR_FUNC_L2(str, type, mem, n1,n2)   \
    extern type* ak_mi_##str##_get_##n1##_##n2##_##ptr(struct str* ptr);

#define AKMI_EXTERN_SET_MEM_FUNC_L2(str, type, mem, n1, n2)   \
    extern void ak_mi_##str##_set_##n1##_##n2(struct str* ptr, type val);
  
/*  
 * Level 3  
 */
#define AKMI_EXTERN_GET_MEM_FUNC_L3(str, type, mem, n1,n2,n3)   \
    extern type ak_mi_##str##_get_##n1##_##n2##_##n3(struct str* ptr);
#define AKMI_EXTERN_GET_MEM_PTR_FUNC_L3(str, type, mem, n1,n2,n3)   \
    extern type* ak_mi_##str##_get_##n1##_##n2##_##n3##_##ptr(struct str* ptr); 
#define AKMI_EXTERN_SET_MEM_FUNC_L3(str, type, mem, n1, n2, n3)   \
    extern void ak_mi_##str##_set_##n1##_##n2##_##n3(struct str* ptr, type val); 


/*************************** MACRO HELPER For call  ***************************/
/*
 *  Level 1  API:
 *  
 */
#define AKMI_GET(str,pstr,mem)  \
    ak_mi_##str##_get_##mem(pstr)

#define AKMI_GET_PTR(str,pstr,mem)  \
    ak_mi_##str##_get_##mem##_##ptr(pstr)


#define AKMI_SET(str, pstr, mem, val)  \
do { \
     ak_mi_##str##_set_##mem(pstr, val); \
}while(0)


/*
 *  Level 2  API:
 * 
 *  struct platform_device *pdev;
 *  struct device_node* np = pdev->dev.of_node;
 * 
 *  Equal to:
 *  struct platform_device *pdev;
 *  struct device_node* np = AKMI_GET_L2(platform_device, pdev, dev ,of_node);
 *  
 */

#define AKMI_GET_L2(str, pstr, n1, n2)  \
    ak_mi_##str##_get_##n1##_##n2(pstr)

#define AKMI_GET_L2_PTR(str, pstr, n1, n2)  \
    ak_mi_##str##_get_##n1##_##n2##_##ptr(pstr)

#define AKMI_SET_L2(str, pstr,n1, n2, val)  \
do { \
     ak_mi_##str##_set_##n1##_##n2(pstr, val); \
}while(0)

/*
 *  Level 3  API:  
 *  
 */

#define AKMI_GET_L3(str, pstr, n1, n2,n3)  \
    ak_mi_##str##_get_##n1##_##n2##_##n3(pstr)  
#define AKMI_GET_L3_PTR(str, pstr, n1, n2)  \
    ak_mi_##str##_get_##n1##_##n2##_##n3##_##ptr(pstr)  

#define AKMI_SET_L3(str, pstr, n1, n2, n3, val)  \
do { \
     ak_mi_##str##_set_##n1##_##n2##_##n3(pstr, val); \
}while(0)



/********************************* SIZE TABLE ********************************/
#include <media/v4l2-dev.h>

/* 
 * struct_size_talbe helper
 */
#define AKMI_SIZE_DEFINE(str)   int str;
#define AKMI_SIZE_INIT(str)     .str = sizeof(struct str),
#define AKMI_SIZEOF(str)        (struct_size_talbe.str)


struct struct_size_talbe 
{
    AKMI_SIZE_DEFINE(iio_dev)  /* size of struct iio_dev */
    AKMI_SIZE_DEFINE(video_device)  /* size of struct video_device*/
};
extern struct struct_size_talbe  struct_size_talbe;

/****************************** AKMI For ak_mci.ko ****************************/
#include <linux/mmc/host.h>
#include <linux/platform_device.h>

AKMI_EXTERN_GET_MEM_FUNC(mmc_host,void *,private) 

AKMI_EXTERN_GET_MEM_FUNC(file,void * ,private_data)
AKMI_EXTERN_SET_MEM_FUNC(file,void * ,private_data)


AKMI_EXTERN_GET_MEM_FUNC_L2(platform_device,void * ,dev.of_node, dev, of_node)
AKMI_EXTERN_GET_MEM_FUNC_L2(platform_device,void * ,dev.pins, dev, pins)

/****************************** AKMI For ak_i2c.ko ****************************/
#include <linux/i2c.h>

/*
 *
 * extern const char * ak_mi_i2c_adapter_get_name(struct i2c_adapter* ptr);
 */
AKMI_EXTERN_GET_MEM_FUNC(i2c_adapter, const char *, name)

/*
 * extern void ak_mi_i2c_adapter_set_nr(struct i2c_adapter* ptr, int val);
 *
 */
AKMI_EXTERN_SET_MEM_FUNC(i2c_adapter, int, nr)

/*
 *
 * extern void ak_mi_i2c_adapter_get_nr(struct i2c_adapter* ptr);
 */
AKMI_EXTERN_GET_MEM_FUNC(i2c_adapter, int, nr)

/***************************** AKMI For ak_motor.ko ***************************/

/*
 *
 * extern struct cdev * ak_mi_inode_get_i_cdev(struct inode* ptr);
 */
AKMI_EXTERN_GET_MEM_FUNC(inode, struct cdev *, i_cdev)

/**************************** AKMI For ak_saradc.ko **************************/
#include <linux/iio/iio.h>

/*
 * extern void ak_mi_device_set_of_node(struct device* ptr,
 * struct device_node * val);
 */
AKMI_EXTERN_SET_MEM_FUNC(device, struct device_node *, of_node)

/***************************** AKMI For ak_i2s.ko ***************************/
AKMI_EXTERN_GET_MEM_FUNC(device, struct device_node *, of_node);

/****************************** AKMI For ak_hcd.ko *************************/
AKMI_EXTERN_GET_MEM_FUNC_L2(platform_device,u64,dev.coherent_dma_mask, \
    dev,coherent_dma_mask);

AKMI_EXTERN_SET_MEM_FUNC_L2(platform_device,u64,dev.coherent_dma_mask, \
    dev,coherent_dma_mask);

AKMI_EXTERN_GET_MEM_PTR_FUNC_L2(platform_device,u64, \
    dev.coherent_dma_mask,dev,coherent_dma_mask);

AKMI_EXTERN_GET_MEM_FUNC_L2(platform_device,u64*,dev.dma_mask, \
    dev,dma_mask);

AKMI_EXTERN_SET_MEM_FUNC_L2(platform_device,u64*,dev.dma_mask, \
    dev,dma_mask);

/****************************** AKMI For ak_udc.ko *************************/
AKMI_EXTERN_GET_MEM_FUNC(platform_device,u32,num_resources);

AKMI_EXTERN_GET_MEM_FUNC(platform_device,struct resource *,resource);

#endif
