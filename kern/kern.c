#include "../XMC1400.h"
#include "../string.h"
//#include "../pit.h"
//#include "../aic.h"
//#include "../pmc.h"

#include "kern_types.h"
#include "kern_mutex.h"
#include "kern_task.h"
#include "kern_queue.h"
#include "kern_sem.h"
#include "kern_event.h"

int debug_ready;
int us_count;
int task_id_counter;
long system_tick_count;
long randseed;
int kern_int_counter;
int kern_enable_switch_context;
int kern_context_switch_request;
int kern_system_state;
int kern_switch_lock;
unsigned int kern_idle_count;
int kern_created_tasks_qty;                  //-- num of created tasks
unsigned int kern_ready_to_run_bmp;

char dynamic_memory_buffer[DYNAMIC_MEMORY_SIZE];
_dinamic_sram dynamic_memory_bitmap;

_TCB *kern_next_task_to_run;     //-- Task to be run after switch context
_TCB *kern_curr_run_task;        //-- Task that is running now

_QUEUE kern_ready_list; //-- all ready to run(RUNNABLE) tasks
_QUEUE kern_create_queue;                //-- all created tasks
_QUEUE kern_wait_timeout_list;           //-- all tasks that wait timeout expiration
_QUEUE kern_locked_mutexes_list;         //-- all locked mutexes
_QUEUE kern_blocked_tasks_list;          //-- for mutexes priority ceiling protocol


void  kern_switch_context_exit(void);
void  kern_switch_context(void);
void  kern_iswitch_context(void);
void kern_task_exit(int attr);

unsigned int kern_cpu_save_sr(void);
void  kern_cpu_restore_sr(unsigned int sr);
void  kern_start_exe(void);
int   kern_chk_irq_disabled(void);

void kern_int_exit(void);   //-- Cortex-M3
int  kern_inside_irq(void);

void kern_arm_disable_interrupts(void);
void kern_arm_enable_interrupts(void);
void kern_cpu_int_enable(void);

int scan_event_state(_EVENT *evf);
int scan_sem_state(_SEM *sem);

_TCB * get_task_by_timer_queque(_QUEUE * que);
_TCB * get_task_by_tsk_queue(_QUEUE * que);
_TCB * get_task_by_block_queque(_QUEUE * que);
_MUTEX * get_mutex_by_mutex_queque(_QUEUE * que);
_MUTEX * get_mutex_by_lock_mutex_queque(_QUEUE * que);
_MUTEX * get_mutex_by_wait_queque(_QUEUE * que);

void  kern_tick_int_processing (void);

//extern _AIC *paic;
//extern _PIT *ppit;
//extern _PMC  *ppmc;

// ----------- malloc -----------
_SEM  malloc_semaphore;
//----------------------------------------------------------------------------
_dinamic_sram    *memory;
//----------------------------------------------------------------------------
// KERN main function (never return)
//----------------------------------------------------------------------------
void kern_start_system (_TCB * task,
                 void (*task_func)(void *param),  //-- task function
                 int priority,                    //-- task priority
                 unsigned int *task_stack_start, //-- task stack first addr in memory (bottom)
                 int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
                 void *param,                    //-- task function parameter
                 int option                       //-- Creation option
                 )
{
    debug_ready = 0;
    us_count = 0;
    system_tick_count = 0;
    kern_switch_lock = 0;
    randseed = 937186357;

    memory = (_dinamic_sram *)&dynamic_memory_bitmap;
    for (int i = 0; i < (DYNAMIC_MEMORY_SIZE / (MEMORY_PAGE_SIZE * 8)); i++)
    {
        memory->bit_block[i] = 0;
        memory->pointer[i].addr = 0;
    }

    kern_sem_create(&malloc_semaphore,1,1);     // memory allocation semaphore
    queue_reset(&kern_ready_list);

    //kern_sem_create(&dma_semaphore,1,1);     // DMA access semaphore

    queue_reset(&kern_create_queue);
    kern_created_tasks_qty = 0;

    kern_system_state = KERN_ST_STATE_NOT_RUN;

    kern_enable_switch_context = 1;
    kern_ready_to_run_bmp = 0;

    kern_context_switch_request = 0;
    kern_int_counter = 0;

    kern_next_task_to_run = NULL;
    kern_curr_run_task    = NULL;

    kern_idle_count = 0;
    task_id_counter = 1;
    //debug_ready = 1;

    queue_reset(&kern_locked_mutexes_list);
    queue_reset(&kern_blocked_tasks_list);
    queue_reset(&kern_wait_timeout_list);

    //-------- user main task ----------
    task->id_task = 0;
    kern_task_create(task,              //-- task TCB
                 task_func,              //-- task function
                 priority,              //-- task priority
                 task_stack_start,
                 task_stack_size,             //-- task stack size (in int,not bytes)
                 param,                              //-- task function parameter
                 KERN_TASK_IDLE                    //-- Creation option
                 );

    task_to_runnable(task);
    kern_curr_run_task = task;

    //pmc_open();
    //ppmc->enable_peripheral_clock(ID_PIOA);
    //ppmc->enable_peripheral_clock(ID_PIOB);
    //ppmc->enable_peripheral_clock(ID_PIOC);
    //ppmc->enable_peripheral_clock(ID_PIOD);
    //ppmc->enable_peripheral_clock(ID_SYS);
    //ppmc->enable_peripheral_clock(ID_IRQ);

    //aic_open();
    //pit_open();
    //ppit->clear();
    //ppit->enable(812);      // suveikinejimo laikas 3125 * 16 taktu clk = 100 MHz  PIT = 0.5ms

    //paic->enable_int(1);    // system interrupt
    //paic->enable_global();
    kern_arm_enable_interrupts();

    #define REG_ARM_SYSCTRL0_SYST_CSR (*(__IO uint32_t*)0xE000E010U) /**< (ARM_SYSCTRL0) SysTick Control and Status Register */
    #define REG_ARM_SYSCTRL0_SYST_RVR (*(__IO uint32_t*)0xE000E014U) /**< (ARM_SYSCTRL0) SysTick Reload Value Register */
	
    //SYSTICKRVR_bit.RELOAD = 0xF423F;          // run sys tick timer 10ms
    //REG_ARM_SYSCTRL0_SYST_RVR = 0x1869F;          // run sys tick timer 1ms
    REG_ARM_SYSCTRL0_SYST_RVR = 4800;          // run sys tick timer 100us
    REG_ARM_SYSCTRL0_SYST_CSR = 7;

   //-- Run OS - first context switch
   kern_start_exe();
}
//----------------------------------------------------------------------------
void SysTick_HandlerFunc (void)
{
    kern_tick_int_processing();
    kern_switch_context();
}
//----------------------------------------------------------------------------
long kern_get_tickcount (void)
{
    return system_tick_count;
}
//----------------------------------------------------------------------------
void kern_stop_multitasking (void)
{
    kern_switch_lock = 1;
}
//----------------------------------------------------------------------------
void kern_start_multitasking (void)
{
    kern_switch_lock = 0;
}
//----------------------------------------------------------------------------
void srandom (long seed)
{
    int i;

    randseed = seed;
    for (i = 0; i < 50; i++)
        random();
}
//----------------------------------------------------------------------------
long random (void)
{
    long x, hi, lo, t;

    if ((x = randseed) == 0)
        x = 123459876;

    hi = x / 127773;
    lo = x % 127773;
    t = 16807 * lo - 2836 * hi;
    if (t < 0)
        t += 0x7fffffff;

    randseed = t;
    return (t);
}
//----------------------------------------------------------------------------
long kern_get_random (void)
{
    srandom(system_tick_count);
    return random();
}
//----------------------------------------------------------------------------
void  kern_tick_int_processing (void)
{
   KERN_INTSAVE_DATA_INT

   _QUEUE *que;
   _TCB *task;
   int *ptr;
   int *ptr_loop = NULL;
   _TCB *kern_tmp_task_to_run;

   KERN_CHECK_INT_CONTEXT_NORETVAL
   kern_idisable_interrupt();

   us_count++;
   if (us_count == 10)
   {
      system_tick_count++;
      us_count = 0;
   }

   kern_tmp_task_to_run = kern_next_task_to_run;
   //priority  = kern_curr_run_task->priority;

  // -------- scan events ------------ //
    que = kern_create_queue.next;

    while(1)
    {
        ptr = (int *)que->next;
        if (ptr != (int *)&kern_create_queue)
        {
            ptr -= 7;
            task = (_TCB *)ptr;
            if (task->pwait_queue != NULL)
            {
                if (task->task_wait_reason == TSK_WAIT_REASON_EVENT)
                    scan_event_state((_EVENT *)task->pwait_queue);

                if (task->task_wait_reason == TSK_WAIT_REASON_SEM)
                    scan_sem_state((_SEM *)task->pwait_queue);
            }
        }
        else
          break;

        if (ptr_loop == NULL)
            ptr_loop = ptr;
        else if (ptr_loop = ptr)
            break;

        que = que->next;
    }

    // -------- scan timeouts ------------ //
      if(!is_queue_empty((_QUEUE*)&kern_wait_timeout_list))
      {
          que = kern_wait_timeout_list.next;
          while (1)
          {
              task = get_task_by_timer_queque((_QUEUE *)que);

              if(task->tick_count != KERN_WAIT_INFINITE)
              {
                  if(task->tick_count > 0)
                  {
                      if (us_count == 0)
                          task->tick_count--;

                      if(task->tick_count == 0) //-- Time out expiried
                      {
                          task_wait_complete((_TCB*)task, TRUE);
                          task->task_wait_rc = ERR_TIMEOUT;
                      }
                  }
              }

              if (que->prev == que->next)
                  break;

              que = que->next;
              if(que == &kern_wait_timeout_list)
                  break;
          }
      }

    // --- switch context stopped ? --- //
    //if (kern_switch_lock == 0)
    //{
        if (kern_tmp_task_to_run == kern_next_task_to_run)
            find_next_task_to_run(kern_next_task_to_run);
    //}

    kern_ienable_interrupt();
}
//----------------------------------------------------------------------------
//  In fact, this task is always in RUNNABLE state
//----------------------------------------------------------------------------
void kern_idle_task_func(void * par)
{
   while(1)
   {
      kern_idle_count++;
   }
}
//----------------------------------------------------------------------------
// Processor specific routine - here for Cortex-M3
// sizeof(void*) = sizeof(int)
//----------------------------------------------------------------------------
unsigned int *kern_stack_init(void * task_func,void * stack_start, void * param)
{
   unsigned int * stk;

	//-- filling register's position in stack - for debugging only

	stk  = (unsigned int *)stack_start; // Load stack pointer

	*stk = 0x01000000L;                       // xPSR
	stk--;
	*stk = ((unsigned int)task_func) | 1;    // Entry Point (1 for THUMB mode)
	stk--;
	*stk = ((unsigned int)kern_task_exit) | 1; // R14 (LR) (1 for THUMB mode)
	stk--;
	*stk = 0x12121212L;                      // R12
	stk--;
	*stk = 0x03030303L;                      // R3
	stk--;
	*stk = 0x02020202L;                      // R2
	stk--;
	*stk = 0x01010101L;                      // R1
	stk--;
	*stk = (unsigned int)param;              // R0 : task's function argument
	stk--;
	*stk = 0x11111111L;                      // R11
	stk--;
	*stk = 0x10101010L;                      // R10
	stk--;
	*stk = 0x09090909L;                      // R9
	stk--;
	*stk = 0x08080808L;                      // R8
	stk--;
	*stk = 0x07070707L;                      // R7
	stk--;
	*stk = 0x06060606L;                      // R6
	stk--;
	*stk = 0x05050505L;                      // R5
	stk--;
	*stk = 0x04040404L;                      // R4

   return stk;
}
//----------------------------------------------------------------------------
int scan_event_state(_EVENT *evf)
{
   _QUEUE * que;
   _QUEUE * tmp_que;
   _TCB * task;
   int fCond;
   int wcflag;
   int fBreak;
   int io_val;
 //--- Scan event wait queue (checking for empty - before function call)

   wcflag = 0;
   que = evf->wait_queue.next;
   fBreak = 0;
   for(;;)
   {
      task = get_task_by_tsk_queue(que);
      io_val = *evf->io_addr;

      //-- cond ---
      if(task->ewait_mode & KERN_EVENT_WCOND_OR)
         fCond = ((io_val & task->ewait_pattern) != 0);
      else
         fCond = ((io_val & task->ewait_pattern) == task->ewait_pattern);

      if(fCond) //-- Condition to finished waiting
      {
         if(que->next == &(evf->wait_queue)) //-- last
         {
            queue_remove_entry(que);
            fBreak = 1;
         }
         else
         {
            tmp_que = que->next;
            queue_remove_entry(que);
            que = tmp_que;
         }
         //----
         task->ewait_pattern = io_val;
         if(task_wait_complete(task,FALSE))
            wcflag = 1;

         if(fBreak)
            break;
      }
      else //-- Check last - when not need queue_remove_entry
      {
         if(que->next == &(evf->wait_queue)) //-- last
            break;
         else
            que = que->next;
      }
   }
   return wcflag;
}
//----------------------------------------------------------------------------
int scan_sem_state(_SEM *sem)
{
   _QUEUE * que;
   _QUEUE * tmp_que;
   _TCB * task;
   int fCond = 0;
   int wcflag;
   int fBreak;
 //--- Scan event wait queue (checking for empty - before function call)

   wcflag = 0;
   que = sem->wait_queue.next;
   fBreak = 0;
   for(;;)
   {
      task = get_task_by_tsk_queue(que);

      if ((us_count == 0) && (task->tick_count > 0))
          task->tick_count--;

      if ((task->tick_count == 0) || (sem->count > 0))      // semaphore wait timeout
          fCond = 1;

      if(fCond) //-- Condition to finished waiting
      {
         if(que->next == &(sem->wait_queue)) //-- last
         {
            queue_remove_entry(que);
            fBreak = 1;
         }
         else
         {
            tmp_que = que->next;
            queue_remove_entry(que);
            que = tmp_que;
         }

         if(task_wait_complete(task,FALSE))
            wcflag = 1;

         if(fBreak)
            break;
      }
      else //-- Check last - when not need queue_remove_entry
      {
         if(que->next == &(sem->wait_queue)) //-- last
            break;
         else
            que = que->next;
      }
   }
   return wcflag;
}
//----------------------------------------------------------------------------
// This functions will works for all processors where
// sizeof(void*) = sizeof(int)
//----------------------------------------------------------------------------
_TCB *get_task_by_timer_queque(_QUEUE * que)
{
   unsigned  int * ptr;
   if(que == NULL)
      return NULL;

   ptr = (unsigned int*)que;
   ptr --;
   ptr -= (sizeof(_QUEUE)/sizeof(void*));
   return (_TCB *)ptr;
}
//----------------------------------------------------------------------------
_TCB *get_task_by_tsk_queue(_QUEUE * que)
{
   unsigned  int * ptr;
   if(que == NULL)
      return NULL;
   ptr = (unsigned int*)que;
   ptr --;
   return (_TCB *)ptr;
}
//----------------------------------------------------------------------------
_TCB * get_task_by_block_queque(_QUEUE * que)
{
   unsigned  int * ptr;
   if(que == NULL)
      return NULL;

   ptr = (unsigned int*)que;
   ptr --;
   ptr -= (sizeof(_QUEUE)/sizeof(void*)) << 1;
   return (_TCB *)ptr;
}

//----------------------------------------------------------------------------
_MUTEX * get_mutex_by_mutex_queque(_QUEUE * que)
{
   unsigned  int * ptr;
   if(que == NULL)
      return NULL;

   ptr = (unsigned int*)que;
   ptr -= (sizeof(_QUEUE)/sizeof(void*));

   return (_MUTEX *)ptr;
}

//----------------------------------------------------------------------------
_MUTEX * get_mutex_by_lock_mutex_queque(_QUEUE * que)
{
   unsigned  int * ptr;
   if(que == NULL)
      return NULL;

   ptr = (unsigned int*)que;
   ptr -= (sizeof(_QUEUE)/sizeof(void*)) << 1; //-- * 2

   return (_MUTEX *)ptr;
}

//----------------------------------------------------------------------------
_MUTEX * get_mutex_by_wait_queque(_QUEUE * que)
{
   unsigned  int * ptr;
   if(que == NULL)
      return NULL;

   ptr = (unsigned int*)que;

   return (_MUTEX *)ptr;
}
//----------------------------------------------------------------------------
// CPU specific routines for Cortex-M3
//
//----------------------------------------------------------------------------
void kern_cpu_int_enable(void)
{
   kern_arm_enable_interrupts();
}

//----------------------------------------------------------------------------
int kern_inside_int(void)
{
	#define REG_ARM_SYSCTRL0_ICSR   (*(__IO uint32_t*)0xE000ED04U) /**< (ARM_SYSCTRL0) Interrupt Control State Register */
	unsigned long vectActive = REG_ARM_SYSCTRL0_ICSR;
	vectActive &= 0x01FF;
	
	if(vectActive != 0)
		return 1;

	return 0;
}
//---------------------------------------------------------
//    long get_free_mem (void)
//---------------------------------------------------------
long get_free_mem (void)
{
    long i,n;
    long mem_block_count = 0;
    char b;

    for (i = 0; i < (DYNAMIC_MEMORY_SIZE / (MEMORY_PAGE_SIZE * 8)); i++)
    {
        b = memory->bit_block[i];
        for (n = 0; n < 8; n++)
        {
            if ((b & 1) == 0)
                mem_block_count++;

            b >>= 1;
            b &= 0x7f;
        }
    }

    return (mem_block_count * MEMORY_PAGE_SIZE);
}
//---------------------------------------------------------
//    long os_get_mem_window_size (long ptr, long sz_get)
//---------------------------------------------------------
long get_mem_window_size (long ptr, long sz_get)
{
    long idx,left;
    long sz;
    char bit_num;
    char is_open,b;

    idx = (ptr >> 3);                   // praskipinam nesveika
    b = memory->bit_block[idx];
    sz = 0;
    is_open = 0;
    bit_num = 0;

    while (b & 1)             // surandam pirma "0"
    {
        b >>= 1;
        b &= 0x7F;
        bit_num++;
    }

    while ((!(b & 1))&&(bit_num < 8))
    {
        if (bit_num == 7)
            is_open++;
        b >>= 1;
        sz++;
        bit_num++;
    }

    if (is_open)
    {
        idx++;

        while (!memory->bit_block[idx])
        {
            sz += 8;
            idx++;
            if (sz > sz_get)
                return sz;
        }

        left = memory->bit_block[idx] ^ 0xff;
        while ((left & 1))
        {
            left >>= 1;
            sz++;
        }
    }

    return sz;
}
//--------------------------------------------
//      long malloc(long sz)
//--------------------------------------------
long malloc(long sz)
{
    long ptr,idx,sz_bak;
    char bit_pos;

    if (debug_ready)
    {
        if (kern_sem_acquire(&malloc_semaphore,1000) != ERR_NO_ERR)
        {
            //printf("malloc semaphore timeout\n\r");
            return 0xffffffff;
        }
    }

    //if (debug_ready)
    //    printf("malloc(%d) - ",sz);

    idx = 0;

    if (sz & 0x0f)
        sz = (sz >> 4) + 1;
    else
        sz = (sz >> 4);

    sz_bak = sz;
    bit_pos = 0;

next_window:

    while (memory->bit_block[idx] == 0xff)
    {
        idx++;
        if (idx >= (DYNAMIC_MEMORY_SIZE / (MEMORY_PAGE_SIZE * 8)))
        {
            if (debug_ready)
                kern_sem_signal(&malloc_semaphore);

            //if (debug_ready)
            //    printf("malloc error\n\r");
            return (long) - 1;
        }
    }

    ptr = (idx << 3);
    bit_pos = memory->bit_block[idx] ^ 0xff;

    while (!(bit_pos & 1))
    {
        bit_pos >>= 1;
        ptr++;
    }

    if (get_mem_window_size(ptr,sz) < sz)
    {
        idx++;
        goto next_window;
    }

    bit_pos = ptr - (idx << 3);
    if (memory->bit_block[idx])                    // sutvarkom nesveikus
    {
        while ((sz)&&(bit_pos < 8))
        {
            memory->bit_block[idx] += (1 << bit_pos);
            bit_pos++;
            sz--;
        }
    }

    if (bit_pos == 8)
    {
        bit_pos = 0;
        idx++;
    }

    while (sz > 8)
    {
        memory->bit_block[idx] = 0xff;
        sz -= 8;
        idx++;
    }

    while (sz)
    {
        memory->bit_block[idx] += (1 << bit_pos);
        bit_pos++;
        sz--;
    }

    idx = 0;
    ptr = (ptr << 4) + (long)dynamic_memory_buffer; ;

    while (memory->pointer[idx].addr)
        idx++;

    if (idx >= (DYNAMIC_MEMORY_SIZE / (MEMORY_PAGE_SIZE * 8)))        // persipilde kesas
    {
        if (debug_ready)
            kern_sem_signal(&malloc_semaphore);
        //printf("malloc error\n\r");
        //free(ptr);
        return 0xffffffff;
    }

    memory->pointer[idx].addr = ptr;
    memory->pointer[idx].size = sz_bak;

    //if (debug_ready)
    //    printf(" pointer 0x%x\n\r",ptr);
    if (debug_ready)
        kern_sem_signal(&malloc_semaphore);

    return ptr;
}
//--------------------------------------------
//      void free(long p)
//--------------------------------------------
void free(long p)
{
    long idx,sz,byte_idx;
    long ptr;

    if (debug_ready)
    {
        if (kern_sem_acquire(&malloc_semaphore,1000) != ERR_NO_ERR)
        {
            //printf("free semaphore timeout\n\r");
            return;
        }
    }

    //if (debug_ready)
    //    printf("free(%x)\n\r",p);

    if ((p == 0) || (p == 0xffffffff))
    {
        if (debug_ready)
            kern_sem_signal(&malloc_semaphore);
        return;
    }

    ptr = p;
    idx = 0;

    while (1)
    {
        if (memory->pointer[idx].addr == ptr)
        {
            memory->pointer[idx].addr = 0;
            ptr = ((ptr - (long)dynamic_memory_buffer) >> 4);
            sz = memory->pointer[idx].size;
            idx = ptr & 7;                        // gaunam elemento indeksa
            byte_idx = ((ptr & 0xfffffff8) >> 3);
            if (idx & 7)                             // nesveika pradzia
            {
                while ((idx)&&(sz))
                {
                    memory->bit_block[byte_idx] -= (1 << idx);
                    if (idx < 7)
                        idx++;
                    else
                        idx = 0;
                    sz--;
                }
                byte_idx++;
            }

            while (sz > 8)                              // valom sveikus
            {
                memory->bit_block[byte_idx] = 0;
                sz -= 8;
                byte_idx++;
            }

            while (sz)
            {
                memory->bit_block[byte_idx] -= (1 << idx);
                idx++;
                sz--;
            }
            break;
        }

        idx++;
        if (idx >= (DYNAMIC_MEMORY_SIZE / (MEMORY_PAGE_SIZE * 8)))
        {
            if (debug_ready)
                kern_sem_signal(&malloc_semaphore);
            return;
        }
    }
    if (debug_ready)
        kern_sem_signal(&malloc_semaphore);
}
