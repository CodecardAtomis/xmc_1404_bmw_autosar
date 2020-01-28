
#include "kern_types.h"
#include "kern_queue.h"
#include "kern_task.h"

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

//----------------------------------------------------------------------------
//   Structure's field sem->id_sem have to be set to 0
//----------------------------------------------------------------------------
int kern_sem_create(_SEM * sem, int start_value, int max_val)
{
   if(sem == NULL) //-- Thanks to Michael Fisher
      return  ERR_WRONG_PARAM;

   if(max_val <= 0 || start_value < 0 || start_value > max_val ) //-- no recreation
   {
      sem->max_count = 0;
      return  ERR_WRONG_PARAM;
   }

   KERN_CHECK_NON_INT_CONTEXT

   queue_reset(&(sem->wait_queue));

   sem->count     = start_value;
   sem->max_count = max_val;
   sem->id_sem    = KERN_ID_SEMAPHORE;

   return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int kern_sem_delete(_SEM * sem)
{
   KERN_INTSAVE_DATA
   _QUEUE * que;
   _TCB * task;

   if(sem == NULL)
      return ERR_WRONG_PARAM;

   if(sem->id_sem != KERN_ID_SEMAPHORE)
      return ERR_NOEXS;


   KERN_CHECK_NON_INT_CONTEXT

   while(!is_queue_empty(&(sem->wait_queue)))
   {
      kern_disable_interrupt();

     //--- delete from the sem wait queue

      que = queue_remove_head(&(sem->wait_queue));
      task = get_task_by_tsk_queue(que);
      if(task_wait_complete(task,TRUE))
      {
         task->task_wait_rc = ERR_DLT;
         kern_enable_interrupt();
         kern_switch_context();
      }
   }

   if(kern_chk_irq_disabled() == 0) // int enable
      kern_disable_interrupt();

   sem->id_sem = 0; // Semaphore not exists now

   kern_enable_interrupt();

   return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
//  Release Semaphore Resource
//----------------------------------------------------------------------------
int kern_sem_signal(_SEM * sem)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code
   _QUEUE * que;
   _TCB * task;


   if(sem == NULL)
      return  ERR_WRONG_PARAM;

   if(sem->max_count == 0)
      return  ERR_WRONG_PARAM;

   if(sem->id_sem != KERN_ID_SEMAPHORE)
      return ERR_NOEXS;


   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   if(!(is_queue_empty(&(sem->wait_queue))))
   {
      //--- delete from the sem wait queue

      que = queue_remove_head(&(sem->wait_queue));
      task = get_task_by_tsk_queue(que);

      if(task_wait_complete(task,TRUE))
      {
         kern_enable_interrupt();
         kern_switch_context();

         return ERR_NO_ERR;
      }
      rc = ERR_NO_ERR;
   }
   else
   {
      if(sem->count < sem->max_count)
      {
         sem->count++;
         rc = ERR_NO_ERR;
      }
      else
         rc = ERR_OVERFLOW;
   }

   kern_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
// Release Semaphore Resource inside Interrupt
//----------------------------------------------------------------------------
int kern_sem_isignal(_SEM * sem)
{
   KERN_INTSAVE_DATA_INT
   int rc;
   _QUEUE * que;
   _TCB * task;


   if(sem == NULL)
      return  ERR_WRONG_PARAM;

   if(sem->max_count == 0)
      return  ERR_WRONG_PARAM;

   if(sem->id_sem != KERN_ID_SEMAPHORE)
      return ERR_NOEXS;


   KERN_CHECK_INT_CONTEXT

   kern_idisable_interrupt();

   if(!(is_queue_empty(&(sem->wait_queue))))
   {
      //--- delete from the sem wait queue

      que = queue_remove_head(&(sem->wait_queue));
      task = get_task_by_tsk_queue(que);

      if(task_wait_complete(task,TRUE))
      {
         kern_ienable_interrupt();

         return ERR_NO_ERR;
      }
      rc = ERR_NO_ERR;
   }
   else
   {
      if(sem->count < sem->max_count)
      {
         sem->count++;
         rc = ERR_NO_ERR;
      }
      else
         rc = ERR_OVERFLOW;
   }

   kern_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
//   Acquire Semaphore Resource
//----------------------------------------------------------------------------
int kern_sem_acquire(_SEM * sem, unsigned long timeout)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code


   if(sem == NULL || timeout == 0)
      return  ERR_WRONG_PARAM;

   if(sem->max_count == 0)
      return  ERR_WRONG_PARAM;

   if(sem->id_sem != KERN_ID_SEMAPHORE)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   if(sem->count >= 1)
   {
      sem->count--;
      rc = ERR_NO_ERR;
   }
   else
   {
      task_curr_to_wait_action(&(sem->wait_queue), TSK_WAIT_REASON_SEM, timeout);
      kern_enable_interrupt();
      kern_switch_context();

      return kern_curr_run_task->task_wait_rc;
   }

   kern_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
//  Acquire(Polling) Semaphore Resource (do not call  in the interrupt)
//----------------------------------------------------------------------------
int kern_sem_acquire_polling(_SEM * sem)
{
   KERN_INTSAVE_DATA
   int rc;


   if(sem == NULL)
      return  ERR_WRONG_PARAM;

   if(sem->max_count == 0)
      return  ERR_WRONG_PARAM;

   if(sem->id_sem != KERN_ID_SEMAPHORE)
      return ERR_NOEXS;


   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   if(sem->count >= 1)
   {
      sem->count--;
      rc = ERR_NO_ERR;
   }
   else
      rc = ERR_TIMEOUT;

   kern_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
// Acquire(Polling) Semaphore Resource inside interrupt
//----------------------------------------------------------------------------
int kern_sem_acquire_ipolling(_SEM * sem)
{
   KERN_INTSAVE_DATA_INT
   int rc;

   if(sem == NULL)
      return  ERR_WRONG_PARAM;

   if(sem->max_count == 0)
      return  ERR_WRONG_PARAM;

   if(sem->id_sem != KERN_ID_SEMAPHORE)
      return ERR_NOEXS;

   KERN_CHECK_INT_CONTEXT

   kern_idisable_interrupt();

   if(sem->count >= 1)
   {
      sem->count--;
      rc = ERR_NO_ERR;
   }
   else
      rc = ERR_TIMEOUT;

   kern_ienable_interrupt();

   return rc;
}


