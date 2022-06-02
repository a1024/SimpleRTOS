#ifndef SYSTEM_H
#define SYSTEM_H
#include"bsp.h"

#define	COUNTOF(ARR)	(sizeof(ARR)/sizeof(*(ARR)))

void* acme_memset(void *dst, unsigned val, unsigned size);
void *acme_memcpy(void *dst, const void *src, unsigned bytesize);
void* acme_memmove(void *dst, const void *src, unsigned bytesize);
void memfill(void *dst, unsigned dstbytes, const void *src, unsigned srcbytes);

#define memset	acme_memset
#define memcpy	acme_memcpy
#define	memmove	acme_memmove

#define print_char	uart_putc
void print(const char *str);
int print_idec(int n);
int print_ihex(unsigned n);
int print_ihex2(unsigned n, int ndigits);
void print_memory(const char *header, const void *start, const void *end, int width);


#define	STACK_LOGBYTESIZE	10		//should be >5
#define STACK_BYTESIZE		(1<<STACK_LOGBYTESIZE)//bytes
#define MAX_TASKS			4

#define MAX_SEMAPHORES		4
void*			OS_newTask(const char *name, int (*proc)(void*), void *param, unsigned short priority);
//TODO: OS_deleteTask(void *hTask);
void			OS_startScheduler(int period, void (*idleHook)(void));

void*			OS_createSemaphore();
void			OS_takeSemaphore(void *hSemaphore);
void			OS_giveSemaphore(void *hSemaphore);

const char*		OS_getTaskName(void *hTask);
unsigned short	OS_getTaskPriority(void *hTask);
void			OS_setTaskPriority(void *hTask, unsigned short priority);

unsigned		OS_getTickCount(void);
void			OS_sleepUntil(unsigned tickstamp);

#endif
