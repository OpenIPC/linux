#ifndef _VOS_KER_IOCTL_H_
#define _VOS_KER_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#if defined(__LINUX) && defined(__KERNEL__)
#include <asm/ioctl.h>
#define _VOS_IO             _IO
#define _VOS_IOR            _IOR
#define _VOS_IOW            _IOW
#define _VOS_IOWR           _IOWR

#elif defined(__FREERTOS)
#define _VOS_IOC_NRBITS     8
#define _VOS_IOC_TYPEBITS   8
#define _VOS_IOC_SIZEBITS   14

#define _VOS_IOC_NRSHIFT    0
#define _VOS_IOC_TYPESHIFT  (_VOS_IOC_NRSHIFT + _VOS_IOC_NRBITS)
#define _VOS_IOC_SIZESHIFT  (_VOS_IOC_TYPESHIFT + _VOS_IOC_TYPEBITS)
#define _VOS_IOC_DIRSHIFT   (_VOS_IOC_SIZESHIFT + _VOS_IOC_SIZEBITS)

#define _VOS_IOC_NONE       0U
#define _VOS_IOC_WRITE      1U
#define _VOS_IOC_READ       2U

#define _VOS_IOC(dir,type,nr,size) \
        (((dir)  << _VOS_IOC_DIRSHIFT) | \
         ((type) << _VOS_IOC_TYPESHIFT) | \
         ((nr)   << _VOS_IOC_NRSHIFT) | \
         ((size) << _VOS_IOC_SIZESHIFT))

#define _VOS_IOC_TYPECHECK(t) (sizeof(t))

#define _VOS_IO(type,nr)        _VOS_IOC(_VOS_IOC_NONE,(type),(nr),0)
#define _VOS_IOR(type,nr,size)  _VOS_IOC(_VOS_IOC_READ,(type),(nr),(_VOS_IOC_TYPECHECK(size)))
#define _VOS_IOW(type,nr,size)  _VOS_IOC(_VOS_IOC_WRITE,(type),(nr),(_VOS_IOC_TYPECHECK(size)))
#define _VOS_IOWR(type,nr,size) _VOS_IOC(_VOS_IOC_READ|_VOS_IOC_WRITE,(type),(nr),(_VOS_IOC_TYPECHECK(size)))

#else //assume this is Linux user-space
#include <sys/ioctl.h>
#define _VOS_IO             _IO
#define _VOS_IOR            _IOR
#define _VOS_IOW            _IOW
#define _VOS_IOWR           _IOWR
#endif

#ifdef __cplusplus
}
#endif

#endif /* _VOS_KER_IOCTL_H_ */

