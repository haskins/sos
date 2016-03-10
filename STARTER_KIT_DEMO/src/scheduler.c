/*
 * Scheduler
 *
 * This manages the threads and ensures fifo-rr.
 *
 *  Original Author: Rafael Otero
 *  Modifiers: Devon Harker, Josh Haskins, Vincent Tennant
 *
 */ 


#include <asf.h>
#include "minios.h"

#ifndef MINITHREAD_H_
#define MINITHREAD_H_
#include "minithread.h"
#endif

extern uint32_t _estack2;
extern STACK2_SIZE;

#define MAX_NUM_OF_THREADS	100 //has to be fixed
#define QUEUE_SIZE 100

static Minithread threads[MAX_NUM_OF_THREADS];
static int curThread = 0;
static int numOfThreads = 0;
static int allocatedStack  = 0;
static bool firstExec = true;
static Minithread theCurrentThread;
static Minithread queue[QUEUE_SIZE];
static int head;
static int tail;

void scheduler(void){
	//this will not execute on first call of scheduler.
	if (theCurrentThread.name != NULL){
		//enqueue old thread.
		queue[tail] = theCurrentThread;
		tail = (tail + 1) % QUEUE_SIZE;
	}
	
	//dequeue new thread.
	theCurrentThread = queue[head];
	head = (head + 1) % QUEUE_SIZE;
}

void startScheduler(){
	
	curThread = 0;
	
	#define MS_TO_TICKS(x) (sysclk_get_cpu_hz()/1000)*(x)
	#define US_TO_TICKS(x) (sysclk_get_cpu_hz()/1000000)*(x)
	SysTick_Config( US_TO_TICKS(900) );
}

//Taken verbatim
static inline void save_context(void){
	uint32_t scratch;
	asm volatile ("MRS %0, psp\n\t"
	"STMDB %0!, {r4-r11}\n\t"
	"MSR psp, %0\n\t"  : "=r" (scratch) );
}

//Taken verbatim
static inline void load_context(void){
	uint32_t scratch;
	asm volatile ("MRS %0, psp\n\t"
	"LDMFD %0!, {r4-r11}\n\t"
	"MSR psp, %0\n\t"  : "=r" (scratch) );
}

void SysTick_Handler(void){
		//-----------------------------------
		// CONTEXT SWITCHING HAPPENS HERE
		//-------------------------------------

		//save software context
		save_context(); //The first time (as in firstExec) it will save context in some unknown place in psp
						//but this will be ignored, so doesnt matter... ok its not unknown, it's in the msp
						//but the next instruction, when firstExec is true,  is load_context() so it's fine.
					
		if( firstExec){ //the first ever executed thread wasn't running yet
			load_context();
			//so cant get it's sp from psp	
			firstExec = false;
			
			//change current thread
			scheduler();
			
			theCurrentThread.execFirstTime = false;
			__set_PSP( theCurrentThread.sp );
			
			return;
		}

		 //save psp
		 theCurrentThread.sp = __get_PSP();
 
		 //change current thread
		 scheduler();

		//restore psp
		__set_PSP( theCurrentThread.sp );
 		
 		//restore software context
		 if( theCurrentThread.execFirstTime ){	 
			//newly created threads have no soft context so we put one
			theCurrentThread.execFirstTime = false;
			save_context();	
		 }
		 
		 load_context();
		  
}



void PendSV_Handler(void){
	//Doing context switching here allows for applications
	//to "yield", meaning they can voluntarily generate a context switch
	//proper yielding requires setting a yielding time though.
	
	//code from systick??
	  
}



void del_process(void){
	//Making use of this "thread-removing function" requires suing a flag 
	//for setting threads as active/unactive...so add a new field in the MiniThread data
	//structure. Basically, create initially as many threads as the system will require, then
	//when a new thread is created look for an unactive thread and set as active. Removing hen requires
	//setting (withi this function) the thread as unactive and then call context switch....
	//The scheduler obviously must look distinguish between active/unactive threads
	
	//1) Set thread as unused
	//2) Do context switch by pending an SVC: SCB->ICSR |= (1<<28);
	
	while(1); //once the context changes, the program will no longer return to this thread

}




int createThread ( void (*startAddress)(void), char *name,  int stackSize ){
		
		//cant create more threads
		if( numOfThreads >= MAX_NUM_OF_THREADS )
			return -1;
			
		threads[numOfThreads].name = name;	
		threads[numOfThreads].execFirstTime = true;
		threads[numOfThreads].bp = (&_estack2 - 0x20) - (allocatedStack + stackSize);
		threads[numOfThreads].sp = threads[numOfThreads].bp - 0x20;  //make space for manually-inserted hardware context
		
		allocatedStack += stackSize; 
		
		//initially the task does not have a hardware context
		//so we insert one ourselves
		uint32_t* sp = threads[numOfThreads].sp;
		
		((uint32_t*)sp)[0] = ((uint32_t) 0); //r0
		((uint32_t*)sp)[1] = ((uint32_t) 0); //r1
		((uint32_t*)sp)[2] = ((uint32_t) 0); //r2
		((uint32_t*)sp)[3] = ((uint32_t) 0); //r3
		((uint32_t*)sp)[4] = ((uint32_t) 0); //r12
		((uint32_t*)sp)[5] = ((uint32_t) del_process); //lr
		((uint32_t*)sp)[6] = ((uint32_t) startAddress); //pc
		((uint32_t*)sp)[7] = ((uint32_t) 0x21000000); //psr

		//enqueues the just created thread.
		queue[tail] = threads[numOfThreads];
		tail = (tail + 1) % QUEUE_SIZE;
		
		numOfThreads++;
}