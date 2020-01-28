#include "kern_types.h"
#include "kern_queue.h"
#include "kern_task.h"
#include "kern_mutex.h"

extern int task_id_counter;
extern volatile int kern_int_counter;
extern volatile int kern_enable_switch_context;
extern volatile int kern_context_switch_request;
extern volatile int kern_system_state;
extern volatile int kern_created_tasks_qty;                  //-- num of created tasks
extern volatile unsigned int kern_ready_to_run_bmp;

extern _TCB * kern_next_task_to_run;     //-- Task to be run after switch context
extern _TCB * kern_curr_run_task;        //-- Task that is running now
extern _QUEUE kern_ready_list;
extern _QUEUE kern_create_queue;                //-- all created tasks
extern _QUEUE kern_wait_timeout_list;           //-- all tasks that wait timeout expiration

void  kern_switch_context_exit(void);
void  kern_switch_context(void);

unsigned int kern_cpu_save_sr(void);
void  kern_cpu_restore_sr(unsigned int sr);
void  kern_start_exe(void);
int   kern_chk_irq_disabled(void);

_TCB *get_task_by_timer_queque(_QUEUE *que);
_TCB *get_task_by_tsk_queue(_QUEUE *que);
_TCB *get_task_by_block_queque(_QUEUE *que);
_MUTEX *get_mutex_by_mutex_queque(_QUEUE *que);
_MUTEX *get_mutex_by_lock_mutex_queque(_QUEUE *que);
_MUTEX *get_mutex_by_wait_queque(_QUEUE *que);

unsigned int *kern_stack_init(void * task_func,void * stack_start, void * param);

extern _QUEUE kern_blocked_tasks_list;

//-----------------------------------------------------------------------------
int kern_task_create(_TCB *task,
                 void (*task_func)(void *param),  //-- task function
                 int priority,                    //-- task priority
                 unsigned int *task_stack_start, //-- task stack first addr in memory (bottom)
                 int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
                 void *param,                    //-- task function parameter
                 int option                       //-- Creation option
                 )
{
   KERN_INTSAVE_DATA
   int rc; //-- return code

   unsigned int *ptr_stack;
   int i;

   //-- Light weight checking of system tasks recreation
   if((priority == 0 && ((option & KERN_TASK_TIMER) == 0)) ||
       (priority == KERN_NUM_PRIORITY-1 && (option & KERN_TASK_IDLE) == 0))
      return ERR_WRONG_PARAM;

   if((priority < 0 || priority > KERN_NUM_PRIORITY-1) ||
        task_stack_size < KERN_MIN_STACK_SIZE ||
        task_func == NULL || task == NULL || task_stack_start == NULL ||
        task->id_task != 0)  //-- recreation
      return ERR_WRONG_PARAM;

   rc = ERR_NO_ERR;

   KERN_CHECK_NON_INT_CONTEXT

   if(kern_system_state == KERN_ST_STATE_RUNNING)
      kern_disable_interrupt();

   //--- Init task TCB

   task->task_func_addr = (void*)task_func;
   task->task_func_param = param;
   
   task->sp_start = (unsigned int*)task_stack_start;  //-- Base address of task stack space
   
   task->sp_size  = task_stack_size;         //-- Task stack size (in bytes)
   task->priority   = priority;          //-- Task base priority
   task->skip_cnt = 0;
   task->activate_count  = 0;                 //-- Activation request count
   task->id_task = KERN_ID_TASK;
   task->id = kern_task_get_id();
   task->tick_count = 0;
   //task->current_sector_read = (-1);
   //task->dir_entry = (void *)malloc(32);    // direntry structure
   //task->sector_buffer = (char *)malloc(512 + 16);

   if (task->id == (-1))
   {
      if(kern_system_state == KERN_ST_STATE_RUNNING)
          kern_enable_interrupt();

      return ERR_OUT_OF_MEM;
   }

      //-- Fill all task stack space by 0xFFFFFFFF - only inside create_task
   for(ptr_stack = task->sp_start,i = 0;i < task->sp_size; i++)
      *ptr_stack-- = KERN_FILL_STACK_VAL;

   task_set_init_state(task);

      //--- Init task stack
   ptr_stack = kern_stack_init(task->task_func_addr,
                             task->sp_start,
                             task->task_func_param);
   task->task_sp = ptr_stack;                //-- Pointer to task top of stack,when not running


   //-- Add task to created task queue
   queue_add_tail(&kern_create_queue,&(task->create_queue));
   kern_created_tasks_qty++;

   if((option & KERN_TASK_START_ON_CREATION) != 0)
      task_to_runnable(task);

   if(kern_system_state == KERN_ST_STATE_RUNNING)
      kern_enable_interrupt();

   return rc;
}
// ---------------------------------------------------------------------------
//   finds unic task id to assign to task
// ---------------------------------------------------------------------------
int kern_task_get_id (void)
{
    /*_QUEUE *que;
    int *ptr;
    _TCB *task;
    int id_match;
    int i;

    for (i = 1; i < 127; i++)
    {
        que = kern_create_queue.next;
        id_match = 0;
        while(1)
        {
            ptr = (int *)que->next;
            if (ptr != (int *)&kern_create_queue)
            {
                ptr -= 7;
                task = (_TCB *)ptr;
                if (task->id == i)
                {
                    id_match++;
                    break;
                }
            }
            else
              break;
            que = que->next;
        }

        if (id_match == 0)
            break;
    }

    if (i < 127)
        return i;

    return (-1);*/

    task_id_counter++;

    return task_id_counter;
}
// ---------------------------------------------------------------------------
//   finds task by id
// ---------------------------------------------------------------------------
_TCB *kern_task_find_id (int id)
{
    _QUEUE *que;
    int *ptr;
    _TCB *task;

    que = kern_create_queue.next;
    while(1)
    {
        ptr = (int *)que->next;
        //printf("find id - %x\n\r",(long)ptr);
        if (ptr != (int *)&kern_create_queue)
        {
            ptr -= 7;
            task = (_TCB *)ptr;
            if (task->id == id)
            {
                ptr += 7;
                return task;
            }
        }
        else
          break;
        que = que->next;
    }

    return NULL;
}
//----------------------------------------------------------------------------
//  If the task is runnable, it is moved to the SUSPENDED state. If the task
//  is in the WAITING state, it is moved to the WAITING­SUSPENDED state.
//----------------------------------------------------------------------------
int kern_task_suspend(_TCB * task)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code

   //-- ToDo: validation for input parameter etc.
   if(task == NULL)
      return  ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   if(task->task_state & TSK_STATE_SUSPEND)
      rc = ERR_OVERFLOW;
   else
   {
      if(task->task_state == TSK_STATE_DORMANT)
         rc = ERR_WSTATE;
      else
      {
         if(task == kern_curr_run_task && kern_enable_switch_context == 0)
            rc =  ERR_WCONTEXT;
         else
         {
            if(task->task_state == TSK_STATE_RUNNABLE)
            {
               task->task_state = TSK_STATE_SUSPEND;
               task_to_non_runnable(task);
               kern_enable_interrupt();
               if(kern_next_task_to_run != kern_curr_run_task && kern_enable_switch_context != 0)
                  kern_switch_context();

               return  ERR_NO_ERR;
            }
            else
            {
               task->task_state |= TSK_STATE_SUSPEND;
               rc = ERR_NO_ERR;
            }
         }
      }
   }

   kern_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int kern_task_resume(_TCB * task)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code

   if(task == NULL)
      return  ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   if(!(task->task_state & TSK_STATE_SUSPEND))
      rc = ERR_WSTATE;
   else
   {
      if(!(task->task_state & TSK_STATE_WAIT)) // Task not in the WAIT-SUSPEND state
      {
         task_to_runnable(task);
         kern_enable_interrupt();
         if(kern_next_task_to_run != kern_curr_run_task && kern_enable_switch_context != 0)
            kern_switch_context();

         return ERR_NO_ERR;
      }
      else  //-- Just remove TSK_STATE_SUSPEND from  task state
      {
         task->task_state &= ~TSK_STATE_SUSPEND;
         rc = ERR_NO_ERR;
      }
   }

   kern_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
//-- Put current running task to sleep mode
//           (TN_WAIT_INFINITE) -
//                the function's time-out interval never elapses.
//  if wakeup_count > 0, wakeup_count--  and running task continues execution
//----------------------------------------------------------------------------
int kern_task_sleep(unsigned int timeout)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code

   if(timeout == 0)
      return  ERR_WRONG_PARAM;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   if(kern_curr_run_task->wakeup_count > 0)
   {
      kern_curr_run_task->wakeup_count--;
      rc = ERR_NO_ERR;
   }
   else
   {
      task_curr_to_wait_action(NULL,TSK_WAIT_REASON_SLEEP,timeout);
      kern_enable_interrupt();
      kern_switch_context();
      return  ERR_NO_ERR;
   }

   kern_enable_interrupt();
   return rc;
}

//----------------------------------------------------------------------------
int kern_task_wakeup(_TCB * task)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code

   if(task == NULL)
      return  ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   if(task->task_state == TSK_STATE_DORMANT)
   {
      rc = ERR_WCONTEXT;
   }
   else
   {
      if((task->task_state & TSK_STATE_WAIT) &&
                 task->task_wait_reason == TSK_WAIT_REASON_SLEEP)
      {
         if(task_wait_complete(task,FALSE))
         {
            kern_enable_interrupt();
            kern_switch_context();
            return ERR_NO_ERR;
         }
         rc = ERR_NO_ERR;
      }
      else
      {
         if(kern_curr_run_task->wakeup_count == 0) //-- if here - task in not in
         {                                              //-- the SLEEP mode
            kern_curr_run_task->wakeup_count++;
            rc = ERR_NO_ERR;
         }
         else
            rc = ERR_OVERFLOW;
      }
   }
   kern_enable_interrupt();
   return rc;
}
//----------------------------------------------------------------------------
int kern_task_iwakeup(_TCB * task)
{
   KERN_INTSAVE_DATA_INT
   int rc; //-- return code

   if(task == NULL)
      return ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   KERN_CHECK_INT_CONTEXT

   kern_idisable_interrupt();

   if(task->task_state == TSK_STATE_DORMANT)
   {
      rc = ERR_WCONTEXT;
   }
   else
   {
      if((task->task_state & TSK_STATE_WAIT) &&
                 task->task_wait_reason == TSK_WAIT_REASON_SLEEP)
      {
         if(task_wait_complete(task,FALSE))
         {
            kern_context_switch_request = TRUE;
            kern_ienable_interrupt();
            return ERR_NO_ERR;
         }
         rc = ERR_NO_ERR;
      }
      else
      {
         if(kern_curr_run_task->wakeup_count == 0) //-- if here - task in not in
         {                                              //-- the SLEEP mode
            kern_curr_run_task->wakeup_count++;
            rc = ERR_NO_ERR;
         }
         else
            rc = ERR_OVERFLOW;
      }
   }

   kern_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int kern_task_activate(_TCB * task)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code

   if(task == NULL)
      return  ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   if(task->task_state == TSK_STATE_DORMANT)
   {
      task_to_runnable(task);
      kern_enable_interrupt();
      if(kern_next_task_to_run != kern_curr_run_task && kern_enable_switch_context != 0)
            kern_switch_context();
      return ERR_NO_ERR;
   }
   else
   {
      if(task->activate_count == 0)
      {
         task->activate_count++;
         rc = ERR_NO_ERR;
      }
      else
         rc = ERR_OVERFLOW;
   }

   kern_enable_interrupt();

   return rc;
}
//----------------------------------------------------------------------------
int kern_task_iactivate(_TCB * task)
{
   KERN_INTSAVE_DATA_INT
   int rc; //-- return code

   if(task == NULL)
      return  ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   KERN_CHECK_INT_CONTEXT

   kern_idisable_interrupt();

   if(task->task_state == TSK_STATE_DORMANT)
   {
      task_to_runnable(task);
      if(kern_next_task_to_run != kern_curr_run_task && kern_enable_switch_context != 0)
         kern_context_switch_request = 1;

      kern_ienable_interrupt();
      return ERR_NO_ERR;
   }
   else
   {
      if(task->activate_count == 0)
      {
         task->activate_count++;
         rc = ERR_NO_ERR;
      }
      else
         rc = ERR_OVERFLOW;
   }

   kern_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int kern_task_release_wait(_TCB * task)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code

   //-- ToDo: validation for input parameter etc.
   if(task == NULL)
      return  ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   if((task->task_state & TSK_STATE_WAIT) == 0)
   {
      rc = ERR_WCONTEXT;
   }
   else
   {
      if(task_wait_complete(task,TRUE))
      {
         kern_enable_interrupt();
         kern_switch_context();
         return ERR_NO_ERR;
      }
      rc = ERR_NO_ERR;
   }

   kern_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int kern_task_irelease_wait(_TCB * task)
{
   KERN_INTSAVE_DATA_INT
   int rc; //-- return code

   //-- ToDo: validation for input parameter etc.
   if(task == NULL)
      return  ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   KERN_CHECK_INT_CONTEXT
   kern_idisable_interrupt();

   if((task->task_state & TSK_STATE_WAIT) == 0)
      rc = ERR_WCONTEXT;
   else
   {
      if(task_wait_complete(task,1))
         kern_context_switch_request = 1;

      rc = ERR_NO_ERR;
   }

   kern_ienable_interrupt();
   return rc;
}

//----------------------------------------------------------------------------
void kern_task_exit(int attr)
{
   _QUEUE * que;
   _MUTEX * mutex;
   _TCB * task;

   unsigned int * ptr_stack;
   //volatile int stack_exp[TN_PORT_STACK_EXPAND_AT_EXIT]; //-- For GCC ver > 4 only

   KERN_CHECK_NON_INT_CONTEXT_NORETVAL    //-- v. 2.5

   kern_cpu_save_sr();  //-- Disable interrupts without saving SPSR

   //-- To use stack_exp[] and avoid warning message
   //stack_exp[0] = (int)tn_cpu_save_sr; //-- For GCC ver > 4 only
   //ptr_stack =(unsigned int *)stack_exp[0];
   //--------------------------------------------------

   while(!is_queue_empty(&(kern_curr_run_task->mutex_queue)))
   {
      que = queue_remove_head(&(kern_curr_run_task->mutex_queue));
      mutex = get_mutex_by_mutex_queque(que);
      do_unlock_mutex(mutex);
   }

   task = kern_curr_run_task;
   task_to_non_runnable(kern_curr_run_task);

   task_set_init_state(task);
   ptr_stack = kern_stack_init(task->task_func_addr,
                             task->sp_start,
                             task->task_func_param);
   task->task_sp = ptr_stack;  //-- Pointer to task top of stack,when not running

   if(task->activate_count > 0)  //-- Cannot exit
   {
      task->activate_count--;
      task_to_runnable(task);
   }
//-- If need - delete task
   if(attr == KERN_EXIT_AND_DELETE_TASK)
   {
      queue_remove_entry(&(task->create_queue));
      kern_created_tasks_qty--;
      task->id_task = 0;
   }
   // interrupts will be enabled inside kern_switch_context_exit()
   kern_switch_context_exit();
}

//-----------------------------------------------------------------------------
int kern_task_terminate(_TCB * task)
{
   KERN_INTSAVE_DATA

   int rc; //-- return code
   unsigned int * ptr_stack;
   _QUEUE * que;
   _MUTEX * mutex;

   //volatile int stack_exp[TN_PORT_STACK_EXPAND_AT_EXIT]; //-- For GCC ver > 4 only

   if(task == NULL)
      return  ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   rc = ERR_NO_ERR;
   if(task->task_state == TSK_STATE_DORMANT ||
        kern_curr_run_task == task) //-- Cannot terminate running task
      rc = ERR_WCONTEXT;
   else
   {
      if(task->task_state == TSK_STATE_RUNNABLE)
         task_to_non_runnable(task);
      else if(task->task_state & TSK_STATE_WAIT)
      {
         //-- Free all queues involved in waiting
         queue_remove_entry(&(task->task_queue));
         if(queue_contains_entry(&kern_blocked_tasks_list,&(task->block_queue)))
            queue_remove_entry(&(task->block_queue));
         //-----------------------------------------
         if(task->tick_count != KERN_WAIT_INFINITE)
            queue_remove_entry(&(task->timer_queue));
      }

     //-- Unlock all mutexes locked by the task

      while(!is_queue_empty(&(task->mutex_queue)))
      {
         que = queue_remove_head(&(task->mutex_queue));
         mutex = get_mutex_by_mutex_queque(que);
         do_unlock_mutex(mutex);
      }


      task_set_init_state(task);
      ptr_stack = kern_stack_init(task->task_func_addr,
                             task->sp_start,
                             task->task_func_param);
      task->task_sp = ptr_stack;  //-- Pointer to task top of stack,when not running

      if(task->activate_count > 0) //-- Cannot terminate
      {
         task->activate_count--;
         task_to_runnable(task);
         kern_enable_interrupt();
         if(kern_next_task_to_run != kern_curr_run_task && kern_enable_switch_context != 0)
            kern_switch_context();
         return ERR_NO_ERR;
      }
   }

   kern_enable_interrupt();

   return rc;
}

//-----------------------------------------------------------------------------
int kern_task_delete(_TCB * task)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code

   if(task == NULL)
      return ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   rc = ERR_NO_ERR;
   for(;;) //-- Single iteration loop
   {
      if(task->task_state != TSK_STATE_DORMANT) //-- Cannot delete not-terminated task
      {
         rc = ERR_WCONTEXT;
         break;
      }

      //free((long)task->dir_entry);
      //free((long)task->sector_buffer);

      queue_remove_entry(&(task->create_queue));
      kern_created_tasks_qty--;
      task->id_task = 0;
      break;
   }
   kern_enable_interrupt();
   return rc;
}

//----------------------------------------------------------------------------
int kern_task_change_priority(_TCB * task, int new_priority)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code

   if(task == NULL)
      return  ERR_WRONG_PARAM;

   if(task->id_task != KERN_ID_TASK)
      return ERR_NOEXS;

   if(new_priority < 0 || new_priority > KERN_NUM_PRIORITY - 2) //-- try to set pri
      return ERR_WRONG_PARAM;                                // reserved by sys

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   if(new_priority == 0)
      new_priority = task->priority;

   rc = ERR_NO_ERR;

   if(task->task_state == TSK_STATE_DORMANT)
      rc = ERR_WCONTEXT;
   else if(task->task_state == TSK_STATE_RUNNABLE)
   {
      if(change_running_task_priority(task,new_priority))
      {
         kern_enable_interrupt();
         kern_switch_context();
         return ERR_NO_ERR;
      }
   }
   else
   {
      task->priority = new_priority;
   }

   kern_enable_interrupt();

   return rc;
}
//----------------------------------------------------------------------------
void find_next_task_to_run(_TCB *next_task_to_run)
{
   _QUEUE *que;
   _TCB *task;
   unsigned int *ptr;
   //int all_list = 0;

   //que = &kern_ready_list;
   ptr = (unsigned int *)next_task_to_run;
   ptr++;
   que = (_QUEUE *)ptr;
   if (que == NULL)
      return;

   for (;;)
   {
      que = que->next;
      if (que != &kern_ready_list)
      {
          task = get_task_by_tsk_queue(que);
          //if (task->tick_count == 0)
          //{
              task->skip_cnt++;
              if (task->skip_cnt >= task->priority)
              {
                  task->skip_cnt = 0;
                  break;
              }
          //}
      }
      else
      {
          /*if (all_list == 0)
              all_list = 1;
          else
              return;
        */

          if (que == que->next)
              return;
      }
   }

   kern_next_task_to_run = task;
}
//-----------------------------------------------------------------------------
void task_to_non_runnable (_TCB *task)
{
    //int priority;
    //_QUEUE * que;

    //priority = task->priority;
    //que = &kern_ready_list;

    //task->skip_cnt = 0;
    queue_remove_entry(&(task->task_queue));
    find_next_task_to_run(task);
    //kern_enable_interrupt();
    //kern_switch_context();

    //kern_context_switch_request = TRUE;
    //kern_switch_context();

   /*if(is_queue_empty(que))
   {
      kern_ready_to_run_bmp &= ~(1<<priority);

      if(kern_ready_to_run_bmp == 0)
         kern_next_task_to_run = NULL;
      else
         find_next_task_to_run();
   }
   else
   {
      if(kern_next_task_to_run == task)
      {
         //kern_next_task_to_run = get_task_by_tsk_queue(que->next);
          find_next_task_to_run();
      }
   }*/
}
//-----------------------------------------------------------------------------
void task_to_runnable(_TCB *task)
{
   //int priority;

   //priority = task->priority;
   task->task_state = TSK_STATE_RUNNABLE;
   task->pwait_queue = NULL;

   queue_add_tail(&kern_ready_list, &task->task_queue);
   find_next_task_to_run(task);

   //kern_context_switch_request = TRUE;
   //kern_switch_context();

   //kern_ready_to_run_bmp |= 1 << priority;

   /*if(kern_next_task_to_run == NULL || (kern_next_task_to_run != NULL &&
          (priority < kern_next_task_to_run->priority)))
   {
      kern_next_task_to_run = task;
   }*/
}

//----------------------------------------------------------------------------
int  task_wait_complete(_TCB * task,int tqueue_remove_enable)
{
   int rc;
   int fl_mutex;
   int curr_priority;
   _MUTEX * mutex;
   _TCB * tmp_task;
   _QUEUE * t_que;

   if(task == NULL)
      return 0;

   rc = 0;

   t_que = NULL;
   if(task->task_wait_reason == TSK_WAIT_REASON_MUTEX_I ||
      task->task_wait_reason == TSK_WAIT_REASON_MUTEX_C ||
      task->task_wait_reason == TSK_WAIT_REASON_MUTEX_C_BLK)
   {
      fl_mutex = TRUE;
      t_que = task->pwait_queue;
   }
   else
      fl_mutex = FALSE;

   if(tqueue_remove_enable == TRUE)
      queue_remove_entry(&(task->task_queue));

   task->pwait_queue = NULL;
   task->task_wait_rc = ERR_NO_ERR;

   if(task->tick_count != KERN_WAIT_INFINITE )
      queue_remove_entry(&(task->timer_queue));

   task->tick_count = KERN_WAIT_INFINITE;

   if(!(task->task_state & TSK_STATE_SUSPEND))
   {
      task_to_runnable(task);
      rc = 1;
   }
   else  //-- remove WAIT state
      task->task_state = TSK_STATE_SUSPEND;

   if(fl_mutex == TRUE)
   {
      mutex = get_mutex_by_wait_queque(t_que);
      if(task->task_wait_reason == TSK_WAIT_REASON_MUTEX_C_BLK)
      {
         tmp_task = task->blk_task;
         //-- v.2.5
         if(queue_contains_entry(&kern_blocked_tasks_list, &(task->block_queue)))
            queue_remove_entry(&(task->block_queue));
      }
      else
         tmp_task = mutex->holder;

      if(tmp_task != NULL)
      {
         //-- if task was blocked by another task and its pri was changed
            //  - recalc current priority
         if(tmp_task->priority < task->priority)
         {
            curr_priority = find_max_blocked_priority(mutex,tmp_task->priority);
            if(set_current_priority(tmp_task,curr_priority))
               rc = 1;
         }
      }
   }

   task->task_wait_reason = 0; //-- v2.5
   return rc;
}
//----------------------------------------------------------------------------
void task_curr_to_wait_action(_QUEUE * wait_que,int wait_reason,unsigned int timeout)
{
   task_to_non_runnable(kern_curr_run_task);
   kern_curr_run_task->task_state = TSK_STATE_WAIT;
   kern_curr_run_task->task_wait_reason = wait_reason;
   kern_curr_run_task->tick_count = timeout;
   //--- Add to  wait queue  - FIFO
   if(wait_que == NULL) //-- Thanks to Vavilov D.O.
   {
      queue_reset(&(kern_curr_run_task->task_queue));
   }
   else
   {
      queue_add_tail(wait_que,&(kern_curr_run_task->task_queue));
      kern_curr_run_task->pwait_queue = wait_que;
   }
   //--- Add to timers queue
   if(timeout != KERN_WAIT_INFINITE)
      queue_add_tail(&kern_wait_timeout_list,&(kern_curr_run_task->timer_queue));

   //task_to_non_runnable(kern_curr_run_task);
}

//----------------------------------------------------------------------------
int change_running_task_priority(_TCB *task, int new_priority)
{
   int rc;
   int old_priority;
   _QUEUE *que;

   rc = 0;
   old_priority = task->priority;
   que = (_QUEUE *)&kern_ready_list;

  //-- remove curr task from any (wait/ready) queue
   queue_remove_entry(&(task->task_queue));
   if(is_queue_empty(que))  //-- If there are no ready tasks for old priority
      kern_ready_to_run_bmp &= ~(1<<old_priority); //-- clear ready bit for old priority

   task->priority = new_priority;

     //-- Add task to the end of ready queue for current priority
   queue_add_tail(&kern_ready_list, &(task->task_queue));
   kern_ready_to_run_bmp |= 1 << new_priority;
   if(kern_next_task_to_run == task)
   {
      if(new_priority >= old_priority)  //-- new less or the same as old
      {
         find_next_task_to_run(task);
         if(kern_next_task_to_run != task)
            rc = kern_enable_switch_context;
      }
   }
   else
   {
      if(new_priority < kern_next_task_to_run->priority) //-- new is higher
      {
         kern_next_task_to_run = task;
         rc = kern_enable_switch_context;  //-- need switch context
      }
   }
   return rc;
}

//----------------------------------------------------------------------------
int set_current_priority(_TCB * task, int priority)
{
   _TCB * curr_task;
   _MUTEX * mutex;
   int rc;
   int old_priority;
   int new_priority;

   //-- transitive priority changing

   // if we have a task A that is blocked by the task B and we changed priority
   // of task A,priority of task B also have to be changed. I.e, if we have
   // the task A that is waiting for the mutex M1 and we changed priority
   // of this task, a task B that holds a mutex M1 now, also needs priority's
   // changing.  Then, if a task B now is waiting for the mutex M2, the same
   // procedure have to be applied to the task C that hold a mutex M2 now
   // and so on.

   rc = 0;
   curr_task = task;
   new_priority = priority;
   for(;;)
   {
      old_priority = curr_task->priority;
      if(old_priority == new_priority)
         break;
      if(curr_task->task_state == TSK_STATE_RUNNABLE)
      {
         rc = change_running_task_priority(curr_task,new_priority);
         break;
      }
      else if(curr_task->task_state & TSK_STATE_WAIT)
      {
         if(curr_task->task_wait_reason == TSK_WAIT_REASON_MUTEX_I ||
            curr_task->task_wait_reason == TSK_WAIT_REASON_MUTEX_C ||
            curr_task->task_wait_reason == TSK_WAIT_REASON_MUTEX_C_BLK)

         {

            //-- Find task that blocks curr task ( holder of mutex,that
              // curr_task wait)
            mutex = get_mutex_by_wait_queque(curr_task->pwait_queue);
            if(curr_task->task_wait_reason == TSK_WAIT_REASON_MUTEX_C_BLK)
               curr_task = curr_task->blk_task;
            else
               curr_task = mutex->holder;

            if(old_priority < new_priority)  // old higher
                 new_priority = find_max_blocked_priority(mutex,curr_task->priority);
         }
         else
         {
            task->priority = new_priority;
            break;
         }
      }
      else
      {
         task->priority = new_priority;
         break;
      }
   }
   return rc;
}

//----------------------------------------------------------------------------
void task_set_init_state(_TCB* task)
{
   queue_reset(&(task->task_queue));
   queue_reset(&(task->timer_queue));
   queue_reset(&(task->create_queue));
   queue_reset(&(task->mutex_queue));
   queue_reset(&(task->block_queue));

   task->pwait_queue = NULL;
   task->blk_task = NULL;

   //task->priority   = task->base_priority;  //-- Task curr priority
   task->task_state = TSK_STATE_DORMANT;    //-- Task state
   task->task_wait_reason = 0;              //-- Reason for waiting
   task->task_wait_rc  = ERR_NO_ERR;
   task->ewait_pattern = 0;                 //-- Event wait pattern
   task->ewait_mode    = 0;                 //-- Event wait mode:  _AND or _OR
   task->data_elem     = NULL;              //-- Store data queue entry,if data queue is full

   task->tick_count = KERN_WAIT_INFINITE;     //-- Remaining time until timeout
   task->wakeup_count   = 0;                //-- Wakeup request count
   task->suspend_count  = 0;                //-- Suspension count

   task->tslice_count = 0;
}
//---

