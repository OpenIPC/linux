#ifndef __CAM_OS_UTIL_LIST_H__
#define __CAM_OS_UTIL_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



struct CamOsListHead_t
{
    struct CamOsListHead_t *pNext, *pPrev;
};

#ifndef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif
#endif


#define CAM_OS_POISON_POINTER_DELTA 0
#define CAM_OS_LIST_POISON1  ((void *) 0x00100100 + CAM_OS_POISON_POINTER_DELTA)
#define CAM_OS_LIST_POISON2  ((void *) 0x00200200 + CAM_OS_POISON_POINTER_DELTA)


#define CAM_OS_CONTAINER_OF(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

#define CAM_OS_LIST_HEAD_INIT(name) { &(name), &(name) }

#define CAM_OS_LIST_ENTRY(ptr, type, member) \
        CAM_OS_CONTAINER_OF(ptr, type, member)

#define CAM_OS_LIST_FOR_EACH_SAFE(pos, n, head) \
        for (pos = (head)->pNext, n = pos->pNext; pos != (head); \
            pos = n, n = pos->pNext)

static inline void _ListDel(struct CamOsListHead_t * pPrev, struct CamOsListHead_t * pNext)
{
    pNext->pPrev = pPrev;
    pPrev->pNext = pNext;
}

static inline void CAM_OS_LIST_DEL(struct CamOsListHead_t *pEntry)
{
    _ListDel(pEntry->pPrev, pEntry->pNext);
    pEntry->pNext = CAM_OS_LIST_POISON1;
    pEntry->pPrev = CAM_OS_LIST_POISON2;
}

static inline void CAM_OS_INIT_LIST_HEAD(struct CamOsListHead_t *pList)
{
    pList->pNext = pList;
    pList->pPrev = pList;
}

static inline void _ListAdd(struct CamOsListHead_t *new,
                              struct CamOsListHead_t *pPrev,
                              struct CamOsListHead_t *pNext)
{
    pNext->pPrev = new;
    new->pNext = pNext;
    new->pPrev = pPrev;
    pPrev->pNext = new;
}

static inline void CAM_OS_LIST_ADD_TAIL(struct CamOsListHead_t *new, struct CamOsListHead_t *head)
{
    _ListAdd(new, head->pPrev, head);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //__CAM_OS_UTIL_LIST_H__