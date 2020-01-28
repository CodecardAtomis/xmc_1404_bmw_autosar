#include "kern_types.h"
#include "kern_queue.h"
#include "kern_task.h"
#include "kern_event.h"

int scan_event_waitqueue(_EVENT * evf);

int kern_inside_int(void);

extern volatile int kern_int_counter;
extern volatile int kern_enable_switch_context;
extern volatile int kern_context_switch_request;
extern volatile int kern_system_state;

extern _TCB * kern_next_task_to_run;     //-- Task to be run after switch context
extern _TCB * kern_curr_run_task;        //-- Task that is running now

void  kern_switch_context_exit(void);
void  kern_switch_context(void);

unsigned int kern_cpu_save_sr(void);
void  kern_cpu_restore_sr(unsigned int sr);
void  kern_start_exe(void);
int   kern_chk_irq_disabled(void);
void kern_int_exit(void);   //-- Cortex-M3

_TCB *get_task_by_timer_queque(_QUEUE * que);
_TCB *get_task_by_tsk_queue(_QUEUE * que);
_TCB *get_task_by_block_queque(_QUEUE * que);
_MUTEX *get_mutex_by_mutex_queque(_QUEUE * que);
_MUTEX *get_mutex_by_lock_mutex_queque(_QUEUE * que);
_MUTEX *get_mutex_by_wait_queque(_QUEUE * que);

//----------------------------------------------------------------------------
int kern_event_create(_EVENT * evf, int attr, int *io_addr, unsigned int pattern)
{
   if(evf == NULL || evf->id_event == KERN_ID_EVENT ||
      (((attr & KERN_EVENT_ATTR_SINGLE) == 0)  &&
        ((attr & KERN_EVENT_ATTR_MULTI) == 0)))
      return ERR_WRONG_PARAM;

   queue_reset(&(evf->wait_queue));
   evf->io_addr = io_addr;
   evf->pattern = pattern;
   evf->attr = attr;
   if((attr & KERN_EVENT_ATTR_CLR) && ((attr & KERN_EVENT_ATTR_SINGLE)== 0))
   {
      evf->attr = KERN_INVALID_VAL;
      return ERR_WRONG_PARAM;
   }
   evf->id_event = KERN_ID_EVENT;

   return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int tn_event_delete(_EVENT * evf)
{
   KERN_INTSAVE_DATA
   _QUEUE * que;
   _TCB * task;
   if(evf == NULL)
      return ERR_WRONG_PARAM;

   if(evf->id_event != KERN_ID_EVENT)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   while(!is_queue_empty(&(evf->wait_queue)))
   {
      if(kern_chk_irq_disabled() == 0) // int enable
         kern_disable_interrupt();

     //--- delete from sem wait queue
      que = queue_remove_head(&(evf->wait_queue));
      task = get_task_by_tsk_queue(que);
      if(task_wait_complete(task,FALSE))
      {
         task->task_wait_rc = ERR_DLT;
         kern_enable_interrupt();
         kern_switch_context();
      }
   }

   if(kern_chk_irq_disabled() == 0) // int enable
      kern_disable_interrupt();

   evf->id_event = 0; // Event not exists now

   kern_enable_interrupt();

   return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int kern_event_wait(_EVENT *evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int *p_flags_pattern,
                    unsigned int timeout)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code
   int fCond;
   int io_val;

   if(evf == NULL || wait_pattern == 0 ||
     p_flags_pattern == NULL || timeout == 0)
      return ERR_WRONG_PARAM;

   if(evf->id_event != KERN_ID_EVENT)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   //-- If event attr is TN_EVENT_ATTR_SINGLE and another task already
   //-- in event wait queue - return ERROR without checking release condition
   if((evf->attr & KERN_EVENT_ATTR_SINGLE) && !is_queue_empty(&(evf->wait_queue)))
   {
      rc = ERR_ILUSE;
   }
   else
   {
      io_val = *evf->io_addr;
       //-- Check release condition
      if(wait_mode & KERN_EVENT_WCOND_OR) // any setted bit is enough for release condition
         fCond = ((io_val & wait_pattern) != 0);
      else // TN_EVENT_WCOND_AND is default mode
         fCond = ((io_val & wait_pattern) == wait_pattern);

      if(fCond)
      {
         *p_flags_pattern = io_val;
         if(evf->attr & KERN_EVENT_ATTR_CLR)
            evf->pattern = 0;
          rc = ERR_NO_ERR;
      }
      else
      {
         kern_curr_run_task->ewait_mode = wait_mode;
         kern_curr_run_task->ewait_pattern = wait_pattern;
         task_curr_to_wait_action(&(evf->wait_queue),TSK_WAIT_REASON_EVENT,timeout);
         kern_enable_interrupt();
         kern_switch_context();

         if(kern_curr_run_task->task_wait_rc == ERR_NO_ERR)
            *p_flags_pattern = kern_curr_run_task->ewait_pattern;

         return kern_curr_run_task->task_wait_rc;
      }
   }

   kern_enable_interrupt();

   return rc;
}
//----------------------------------------------------------------------------
int kern_event_wait_polling(_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code
   int fCond;
   int io_val;

   if(evf == NULL || wait_pattern == 0 || p_flags_pattern == NULL)
      return ERR_WRONG_PARAM;

   if(evf->id_event != KERN_ID_EVENT)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   //-- If event attr is TN_EVENT_ATTR_SINGLE and another task already
   //-- in event wait queue - return ERROR without checking release condition
   if((evf->attr & KERN_EVENT_ATTR_SINGLE) && !is_queue_empty(&(evf->wait_queue)))
   {
      rc = ERR_ILUSE;
   }
   else
   {
      io_val = *evf->io_addr;
       //-- Check release condition
      if(wait_mode & KERN_EVENT_WCOND_OR) // any setted bit is enough for release condition
         fCond = ((io_val & wait_pattern) != 0);
      else // TN_EVENT_WCOND_AND is default mode
         fCond = ((io_val & wait_pattern) == wait_pattern);

      if(fCond)
      {
         *p_flags_pattern = io_val;
         if(evf->attr & KERN_EVENT_ATTR_CLR)
            evf->pattern = 0;
          rc = ERR_NO_ERR;
      }
      else
         rc = ERR_TIMEOUT;
   }

   kern_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int kern_event_iwait(_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern)
{
   KERN_INTSAVE_DATA_INT
   int rc; //-- return code
   int fCond;
   int io_val;

   if(evf == NULL || wait_pattern == 0 || p_flags_pattern == NULL)
      return ERR_WRONG_PARAM;

   if(evf->id_event != KERN_ID_EVENT)
      return ERR_NOEXS;

   KERN_CHECK_INT_CONTEXT

   kern_idisable_interrupt();

   //-- If event attr is TN_EVENT_ATTR_SINGLE and another task already
   //-- in event wait queue - return ERROR without checking release condition
   if((evf->attr & KERN_EVENT_ATTR_SINGLE) && !is_queue_empty(&(evf->wait_queue)))
   {
      rc = ERR_ILUSE;
   }
   else
   {
      io_val = *evf->io_addr;
       //-- Check release condition
      if(wait_mode & KERN_EVENT_WCOND_OR) // any setted bit is enough for release condition
         fCond = ((io_val & wait_pattern) != 0);
      else // TN_EVENT_WCOND_AND is default mode
         fCond = ((io_val & wait_pattern) == wait_pattern);

      if(fCond)
      {
         *p_flags_pattern = io_val;
         if(evf->attr & KERN_EVENT_ATTR_CLR)
            evf->pattern = 0;
          rc = ERR_NO_ERR;
      }
      else
         rc = ERR_TIMEOUT;
   }

   kern_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int kern_event_set(_EVENT * evf, unsigned int pattern)
{
   KERN_INTSAVE_DATA

   if(evf == NULL || pattern == 0)
      return ERR_WRONG_PARAM;

   if(evf->id_event != KERN_ID_EVENT)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   evf->pattern |= pattern;

   if(!(is_queue_empty(&(evf->wait_queue))))
   {
      if(scan_event_waitqueue(evf)) // There are task(s) that waiting  state is complete
      {
         if(evf->attr & KERN_EVENT_ATTR_CLR)
            evf->pattern = 0;

         kern_enable_interrupt();
         kern_switch_context();
         return ERR_NO_ERR;
      }
   }
   kern_enable_interrupt();
   return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int kern_event_iset(_EVENT * evf, unsigned int pattern)
{
   KERN_INTSAVE_DATA_INT

   if(evf == NULL || pattern == 0)
      return ERR_WRONG_PARAM;

   if(evf->id_event != KERN_ID_EVENT)
      return ERR_NOEXS;

   KERN_CHECK_INT_CONTEXT

   kern_idisable_interrupt();

   evf->pattern |= pattern;

   if(!(is_queue_empty(&(evf->wait_queue))))
   {
      if(scan_event_waitqueue(evf)) // There are task(s) that waiting  state is complete
      {
         if(evf->attr & KERN_EVENT_ATTR_CLR)
            evf->pattern = 0;

         kern_context_switch_request = TRUE;
         kern_ienable_interrupt();
         return ERR_NO_ERR;
      }
   }
   kern_ienable_interrupt();
   return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int kern_event_clear(_EVENT * evf, unsigned int pattern)
{
   KERN_INTSAVE_DATA

   if(evf == NULL || pattern == 0xFFFFFFFF)
      return ERR_WRONG_PARAM;

   if(evf->id_event != KERN_ID_EVENT)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   evf->pattern &= pattern;

   kern_enable_interrupt();
   return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int kern_event_iclear(_EVENT * evf, unsigned int pattern)
{
   KERN_INTSAVE_DATA_INT

   if(evf == NULL || pattern == 0xFFFFFFFF)
      return ERR_WRONG_PARAM;

   if(evf->id_event != KERN_ID_EVENT)
      return ERR_NOEXS;

   KERN_CHECK_INT_CONTEXT

   kern_idisable_interrupt();

   evf->pattern &= pattern;

   kern_ienable_interrupt();
   return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int scan_event_waitqueue(_EVENT *evf)
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
      else // TN_EVENT_WCOND_AND is default mode
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





