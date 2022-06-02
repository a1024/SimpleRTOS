#define BSP_IMPL
#include"system.h"

extern unsigned long __sys_stack[256];

void* acme_memset(void *dst, unsigned val, unsigned size)
{
	const int unit=sizeof(long long);
	unsigned a1=(unsigned)dst, a2=a1+size;
	unsigned aa1=(a1&~(unit-1))+(((a1&(unit-1))!=0)<<2)+unit, aa2=a2&~(unit-1);
	if(a2<aa1)
	{
		for(;a1<<a2;++a1)
			*(unsigned char*)a1=val;
	}
	else
	{
		for(;a1<<aa1;++a1)
			*(unsigned char*)a1=val;
		unsigned temp=*(unsigned*)(aa1-unit);
		for(;aa1<aa2;aa1+=unit)
			*(unsigned*)aa1=temp;
		for(;aa1<a2;++aa1)
			*(unsigned char*)aa1=val;
	}
	return dst;
}
void *acme_memcpy(void *dst, const void *src, unsigned bytesize)
{
	unsigned char *p1=(unsigned char*)dst;
	const unsigned char *p2=(const unsigned char*)src;
	for(int k=0;k<bytesize;++k)//naive implementation
		p1[k]=p2[k];
	return dst;
}
void* acme_memmove(void *dst, const void *src, unsigned bytesize)
{
	unsigned char *p1=(unsigned char*)dst;
	const unsigned char *p2=(const unsigned char*)src;
	if(dst<src)
	{
		for(int k=0;k<bytesize;++k)//naive implementation
			p1[k]=p2[k];
	}
	else if(src<dst)
	{
		for(int k=bytesize-1;k>=0;--k)//naive implementation
			p1[k]=p2[k];
	}
	return dst;
}
void memfill(void *dst, unsigned dstbytes, const void *src, unsigned srcbytes)
{
	unsigned copied;
	char *d=(char*)dst;
	const char *s=(const char*)src;
	if(dstbytes<srcbytes)
	{
		acme_memcpy(dst, src, dstbytes);
		return;
	}
	copied=srcbytes;
	acme_memcpy(d, s, copied);
	while(copied<<1<=dstbytes)
	{
		acme_memcpy(d+copied, d, copied);
		copied<<=1;
	}
	if(copied<dstbytes)
		acme_memcpy(d+copied, d, dstbytes-copied);
}

void print(const char *str)
{
	while(*str)
	{
		print_char(*str);
		++str;
	}
}
char buf[16];
int print_idec(int n)
{
	int printed=0, idx=0;
	if(n<0)
	{
		print_char('-');
		++printed;
		n=-n;
	}
	do
	{
		buf[idx]='0'+n%10;
		++idx;
		n/=10;
	}
	while(n);
	for(int k=idx;k>=0;--k)//reverse
		print_char(buf[k]);
	printed+=idx;
	return printed;
}
int print_ihex(unsigned n)
{
	print_char('0');
	print_char('x');
	for(int k=32-4;k>=0;k-=4)
	{
		char c=n>>k&15;
		if(c<10)
			print_char('0'+c);
		else
			print_char('A'-10+c);
	}
	return 10;
}
int print_ihex2(unsigned n, int ndigits)
{
	int printed=0;
	--ndigits;
	ndigits<<=2;
	for(int k=32-4;k>=0;k-=4)
	{
		char c=n>>k&15;
		if(printed||c||k==ndigits)
		{
			if(c<10)
				print_char('0'+c);
			else
				print_char('A'-10+c);
			++printed;
		}
	}
	if(!printed)
		print_char('0'), ++printed;
	return printed;
}
static void print_mem_idxrow(int count)
{
	print_char('\n');
	for(int k=0;k<12;++k)
		print_char(' ');
	for(int k=0;k<count;++k)
	{
		int printed=print_ihex2(k<<2, 2);
		printed=11-printed;
		for(int k2=0;k2<printed;++k2)
			print_char(' ');
	}
	print_char('\n');
}
void print_memory(const char *header, const void *start, const void *end, int width)
{
	if(header)
		print(header);
	if(end<=start)
		return;
	unsigned p1=(unsigned)start, p2=(unsigned)end;
	width<<=2;

	p1-=p1%width;
	p2=(p2/width+(p2%width!=0))*width;
	int need_header=0;
	print_mem_idxrow(width>>2);
	for(;p1<p2;)
	{
		if(!(p1&0x1FF)&&need_header)
			print_mem_idxrow(width>>2);
		print_ihex(p1), print_char(':');
		for(unsigned p3=p1+width;p1<p2&&p1<p3;p1+=4)
			print(" "), print_ihex(*(int*)p1);
		print_char('\n');
		need_header=1;
	}
	print_char('\n');
}




//intrernal OS header
typedef enum TaskStateEnum
{
	TASK_NEW,
	TASK_RUNNING,
	TASK_READY,
	TASK_WAITING,//time deay
	TASK_WAITING_ACQUIRE_SEM,//semaphore, check hResource
	TASK_WAITING_RELEASE_SEM,
	TASK_TERMINATED,
} TaskState;
typedef struct TaskContextStruct//64 bytes, 4-byte aligned
{
	unsigned
		*stp,					// 0	stp must be first
		*stbase,				// 4
		*sttop;					// 8
	int (*proc)(void*);			//12
	void *input;				//16
	unsigned waketime;			//20
	const char *name;			//24
	unsigned short priority;	//28
	TaskState state;			//30
	unsigned char reserved;		//31
	unsigned *hResource;
	unsigned reserved2[31];
} TaskContext;
extern volatile unsigned OS_tickCounter, OS_switchCounter;
extern TaskContext *volatile OS_currentTask, *volatile OS_nextTask;
void handler_PendSV(void);
void handler_SysTick(void);
unsigned char* OS_getStackPointer(void);




volatile int panic_flag=0;
volatile unsigned panic_registers[17];
const char *register_names[]=
{
	"r0", "r1", "r2", "r3",
	"r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11",
	"r12",
	"r13 (sp)", "r14 (lr)", "r15 (pc)", "psr"
};
void copy_registers(void);//doesn't work
void print_registers(void)//doesn't work
{
	copy_registers();
	print("Registers:\n");
	for(int k=0;k<16;++k)
	{
		print("\t");
		print(register_names[k]);
		print("\t= ");
		print_ihex(panic_registers[k]);
		print("\n");
	}
	print("\n");
}
void panic(void);

#define MEM_LIMIT (STACK_BYTESIZE)*(MAX_TASKS)
__attribute__((aligned(32))) static unsigned char memory_tasks[MEM_LIMIT];//1024 bytes * 4 tasks = 4096 bytes
static unsigned char *global_pointer=memory_tasks;

static TaskContext *taskHandles[MAX_TASKS];
static int ntasks=0;

TaskContext *volatile OS_currentTask=0, *volatile OS_nextTask=0;

static void (*OS_idleHook)(void)=0;
TaskContext* OS_getCurrentTaskHandle(void)//use OS_currentTask instead
{
	unsigned char *stackpointer=OS_getStackPointer();
	int offset=stackpointer-memory_tasks;
	offset&=~(STACK_BYTESIZE-1);
	return (TaskContext*)(memory_tasks+offset);
}
volatile unsigned OS_tickCounter=0, OS_switchCounter=0;
void		OS_compateTasks(int *target, int test)
{
	if(*target==-1||taskHandles[*target]->priority<taskHandles[test]->priority)
		*target=test;
}
static TaskContext* OS_selectTask(void)
{
	asm("cpsid i");//sample and hold
	int idx=-1;
	for(int i=0;i<ntasks;++i)//find highest priority task
	{
		TaskContext *task=taskHandles[i];
		switch(task->state)
		{
		case TASK_NEW:
			OS_compateTasks(&idx, i);
			break;
		case TASK_RUNNING:
			task->state=TASK_READY;
		case TASK_READY:
			OS_compateTasks(&idx, i);
			break;
		case TASK_WAITING:
			if(OS_tickCounter>=task->waketime)
				OS_compateTasks(&idx, i);
			break;
		case TASK_WAITING_ACQUIRE_SEM:
			if(*task->hResource>0)
				OS_compateTasks(&idx, i);
			break;
		case TASK_WAITING_RELEASE_SEM:
			if(!*task->hResource)
				OS_compateTasks(&idx, i);
			break;
		}
	/*	if(task->state==TASK_WAITING_ACQUIRE_SEM&&!task->hResource)
			task->state=TASK_READY;
		if(task->state==TASK_WAITING_RELEASE_SEM&&task->hResource)
			task->state=TASK_READY;
		if(task->state==TASK_WAITING&&OS_tickCounter>=task->waketime)//TODO: handle integer overflow
			task->state=TASK_READY;
		if(task->state==TASK_NEW||task->state==TASK_READY||task->state==TASK_RUNNING)
		{
			if(idx==-1||taskHandles[idx]->priority<task->priority)
				idx=i;
		}//*/
	}
	asm("cpsie i");
	if(idx==-1)
		return 0;
	return taskHandles[idx];
}
static void OS_switchContext(void)
{
	OS_nextTask=OS_selectTask();
	TaskContext *temp=OS_currentTask;
	if(!OS_nextTask)
	{
		OS_currentTask=0;
		if(OS_idleHook)
			OS_idleHook();
		do
			OS_nextTask=OS_selectTask();
		while(!OS_nextTask);
		OS_currentTask=temp;
	}
	if(OS_nextTask!=OS_currentTask)
	{
		if(OS_currentTask&&OS_currentTask->state==TASK_RUNNING)
			OS_currentTask->state=TASK_READY;
		OS_nextTask->state=TASK_RUNNING;
		PendSV_trigger();
	}
}
static void OS_procCaller(void)
{
	TaskContext *task=OS_getCurrentTaskHandle();
	print("OS: Starting ");
	print(task->name);
	print("...\n");
	int retcode=task->proc(task->input);
	print("\nOS: ");
	print(task->name);
	print(" quit and returned ");
	print_ihex(retcode);
	print("\n");
	//OS_setTaskPriority(0, 0);//TODO: delete task instead
	for(unsigned t=OS_getTickCount();;)
	{
		t+=0xFFFF;
		OS_sleepUntil(t);
	}
}
void* OS_newTask(const char *name, int (*proc)(void*), void *param, unsigned short priority)
{
	unsigned uninit_val=ntasks+1;
	if(ntasks>=MAX_TASKS)
		return 0;
	if(global_pointer>=memory_tasks+MEM_LIMIT)
		return 0;
	TaskContext *task=(TaskContext*)global_pointer;
	memset(task, 0, sizeof(TaskContext));
	task->sttop=(unsigned*)(global_pointer+STACK_BYTESIZE);
	task->stbase=(unsigned*)(global_pointer+sizeof(TaskContext));
	global_pointer+=STACK_BYTESIZE;
	
	task->proc=proc;
	task->input=param;
	
	task->name=name;
	
	task->priority=priority;
	task->state=TASK_NEW;
	
	taskHandles[ntasks]=task;
	++ntasks;
	
	unsigned *p=task->sttop;		//initialize memory
	uninit_val=uninit_val<<16|0xBEEF;
	*(--p)=1<<24;	//PSR
	*(--p)=(unsigned)OS_procCaller;//PC
	*(--p)=0x0D<<24|uninit_val;//LR
	*(--p)=0x0C<<24|uninit_val;//R12
	*(--p)=0x03<<24|uninit_val;//R3
	*(--p)=0x02<<24|uninit_val;//R2
	*(--p)=0x01<<24|uninit_val;//R1
	*(--p)=0x00<<24|uninit_val;//R0
	
	*(--p)=0x0B<<24|uninit_val;//R11
	*(--p)=0x0A<<24|uninit_val;//R10
	*(--p)=0x09<<24|uninit_val;//R9
	*(--p)=0x08<<24|uninit_val;//R8
	*(--p)=0x07<<24|uninit_val;//R7
	*(--p)=0x06<<24|uninit_val;//R6
	*(--p)=0x05<<24|uninit_val;//R5
	*(--p)=0x04<<24|uninit_val;//R4
	task->stp=p;
	uninit_val|=0xFF000000;
	for(p=task->stbase;p<task->stp;++p)
		*p=uninit_val;
		//*p=0xCCCCCCCC;
		//*p=0xDEADBEEF;
		//*p=0x33221100;
	return (void*)task;
}
void OS_startScheduler(int period, void (*idleHook)(void))
{
	OS_idleHook=idleHook;
	
	OS_currentTask=0;
	PendSV_setPriority(7);//lowest priority
	systick_init(period);
	
	OS_switchContext();
	for(;;);
}

static unsigned memory_semaphores[MAX_SEMAPHORES];//1: not taken, 0: taken
int			semaphore_count=0;
void*		OS_createSemaphore()
{
	if(semaphore_count>=MAX_SEMAPHORES)
		return 0;
	memory_semaphores[semaphore_count]=0;//initiallly empty
	++semaphore_count;
	return memory_semaphores+semaphore_count-1;
}
void		OS_takeSemaphore(void *hSemaphore)
{
	unsigned *semaphore;

	interrupts_disable();
	semaphore=(unsigned*)hSemaphore;
	if(!*semaphore)
	{
		OS_currentTask->state=TASK_WAITING_ACQUIRE_SEM;
		OS_currentTask->hResource=semaphore;
		interrupts_enable();
		OS_switchContext();
		interrupts_disable();
	}
	if(!*semaphore)
		panic();
	--*semaphore;
	interrupts_enable();
	OS_switchContext();
}
void		OS_giveSemaphore(void *hSemaphore)
{
	unsigned *semaphore;

	interrupts_disable();
	semaphore=(unsigned*)hSemaphore;
	if(*semaphore)
	{
		OS_currentTask->state=TASK_WAITING_RELEASE_SEM;
		OS_currentTask->hResource=semaphore;
		interrupts_enable();
		OS_switchContext();
		interrupts_disable();
	}
	if(*semaphore)
		panic();
	++*semaphore;
	interrupts_enable();
	OS_switchContext();
}

const char* OS_getTaskName(void *hTask)
{
	TaskContext *task=(TaskContext*)hTask;
	if(!task)
		task=OS_getCurrentTaskHandle();
	return task->name;
}
unsigned short OS_getTaskPriority(void *hTask)
{
	TaskContext *task=(TaskContext*)hTask;
	if(!task)
		task=OS_getCurrentTaskHandle();
	return task->priority;
}
void OS_setTaskPriority(void *hTask, unsigned short priority)
{
	TaskContext *task=(TaskContext*)hTask;
	if(!task)
		task=OS_getCurrentTaskHandle();
	task->priority=priority;
	OS_switchContext();
}
unsigned OS_getTickCount(void)
{
	return OS_tickCounter;
}
void OS_sleepUntil(unsigned tickstamp)
{
	TaskContext *task=OS_getCurrentTaskHandle();
	task->state=TASK_WAITING;
	task->waketime=tickstamp;
	OS_switchContext();
}

void handler_SysTick(void)
{
	asm("cpsid i");
	++OS_tickCounter;
	asm("cpsie i");
	if(OS_currentTask)
		OS_switchContext();
}
void breakpoint(void)
{
	unsigned char *sp=OS_getStackPointer();
	print_ihex((int)sp+16), print(" <- sp\n");
	print_memory("sp[8]:", sp, sp+32, 8);
}
void panic(void)
{
	panic_flag=1;
	print_memory("__sys_stack:", __sys_stack, __sys_stack+COUNTOF(__sys_stack), 8);
	print_memory("memory_tasks:", memory_tasks, memory_tasks+COUNTOF(memory_tasks), 8);
	unsigned sp=(unsigned)OS_getStackPointer();
	print_ihex(sp), print(" <- sp\n\n");
}
