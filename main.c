#include"system.h"
void print_sizes()
{
#define PRINT_SIZE(TYPE)	print_idec(sizeof(TYPE)), print(" = sizeof(" #TYPE ")\n");
	PRINT_SIZE(char);
	PRINT_SIZE(short);
	PRINT_SIZE(int);
	PRINT_SIZE(long);
	PRINT_SIZE(long long);
	PRINT_SIZE(float);
	PRINT_SIZE(double);
	PRINT_SIZE(long double);
#undef PRINT_SIZE
	print_char('\n');
}

void *semaphore=0;
int proc_emitter(void *param)
{
	unsigned timestamp=0;
	const char *name=OS_getTaskName(0);
	for(;;)
	{
		timestamp=OS_getTickCount();

		print("Hello from ");
		print(name);
		print("! Time = ");
		print_ihex(timestamp);
		print(", param = ");
		print_ihex((int)param);
		print("\n");
		
		OS_giveSemaphore(semaphore);

		OS_sleepUntil(timestamp+3);
	}
}
int proc_handler(void *param)
{
	unsigned timestamp=0;
	const char *name=OS_getTaskName(0);
	OS_takeSemaphore(semaphore);
	for(;;)
	{
		timestamp=OS_getTickCount();

		print("Hello from ");
		print(name);
		print("! Time = ");
		print_ihex(timestamp);
		print(", param = ");
		print_ihex((int)param);
		print("\n");

		OS_takeSemaphore(semaphore);

		//OS_sleepUntil(timestamp+3);
	}
}
void idleHook(void)
{
	static int idle_count=0;
	print("Idle ");
	print_idec(idle_count);
	print("\n");
	++idle_count;
}
int main(void)
{
//	print_sizes();
	print("Initializing tasks...\r\n");

	OS_newTask(proc_emitter, (void*)100, 1024, 1, "Periodic 1");
	OS_newTask(proc_emitter, (void*)200, 1024, 1, "Periodic 2");
	OS_newTask(proc_handler, (void*)300, 1024, 3, "Handler");
	semaphore=OS_createSemaphore();

	OS_startScheduler(0x03FFFF, idleHook);
	
	for(;;);
}
