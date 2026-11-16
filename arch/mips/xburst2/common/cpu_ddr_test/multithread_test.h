#ifndef _MULTITHREAD_TEST_H_
#define _MULTITHREAD_TEST_H_

typedef int (*TEST_FUNC)(void *);
int multithread_test_add_func(void *handle,const char *name,TEST_FUNC testfunc,void *param);
void* multithread_test_init(const char *name,int threadcount);
void multithread_test_deinit(void* handle);

#endif /* _MULTITHREAD_TEST_H_ */
