#include "kern_types.h"
#include "kern_queue.h"
#include "kern_task.h"

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

//void kern_arm_disable_interrupts(void);
//void kern_arm_enable_interrupts(void);

_TCB * get_task_by_timer_queque(_QUEUE * que);
_TCB * get_task_by_tsk_queue(_QUEUE * que);
_TCB * get_task_by_block_queque(_QUEUE * que);
_MUTEX * get_mutex_by_mutex_queque(_QUEUE * que);
_MUTEX * get_mutex_by_lock_mutex_queque(_QUEUE * que);
_MUTEX * get_mutex_by_wait_queque(_QUEUE * que);

//----------------------------------------------------------------------------
//-- Circular double-linked list queue
//----------------------------------------------------------------------------

//-- queue reset(init)
void queue_reset(_QUEUE *que)
{
   que->prev = que->next = que;
}

//-- queue_empty
int is_queue_empty(_QUEUE *que)
{
   if(que->next == que && que->prev == que)
      return TRUE;

   return FALSE;
}

//-- Insert entry at head of queue.
void queue_add_head(_QUEUE * que, _QUEUE * entry)
{
   entry->next = que->next;
   entry->prev = que;
   entry->next->prev = entry;
   que->next = entry;
}

//-- Insert entry at tail of queue.
void queue_add_tail(_QUEUE * que, _QUEUE * entry)
{
   entry->next = que;
   entry->prev = que->prev;
   entry->prev->next = entry;
   que->prev = entry;
}

//-- Remove and return entry at head of queue.
_QUEUE * queue_remove_head(_QUEUE * que)
{
   _QUEUE * entry;

   if(que == NULL || que->next == que)
      return (_QUEUE *) 0;

   entry = que->next;
   entry->next->prev = que;
   que->next = entry->next;

   return entry;
}

//-- Remove and return entry at tail of queue.
_QUEUE * queue_remove_tail(_QUEUE * que)
{
   _QUEUE * entry;

   if(que->prev == que)
      return (_QUEUE *) 0;

   entry = que->prev;
   entry->prev->next = que;
   que->prev = entry->prev;

   return entry;
}
//------ Remove entry of queue.
void queue_remove_entry(_QUEUE * entry)
{
   entry->prev->next = entry->next;
   entry->next->prev = entry->prev;
}
//------ Is entry in queue
int queue_contains_entry(_QUEUE * que, _QUEUE * entry)
{
   int result = 0;
   _QUEUE * curr_que;

   curr_que = que->next;
   for(;;) //-- for each task in list
   {
      if(curr_que == entry)
      {
         result = 1; //-- Found
         break;
      }
      if(curr_que->next == que) //-- last
         break;
      else
         curr_que = curr_que->next;
   }
   return result;
}

//---------------------------------------------------------------------------
//-------- Data queue storage FIFO processing -------------------------------
//---------------------------------------------------------------------------
int  dque_fifo_write(_DQUE *dque, void *data_ptr)
{
   register int flag;

   if(dque == NULL)
      return ERR_WRONG_PARAM;

   if(dque->num_entries <= 0)
      return ERR_OUT_OF_MEM;

   flag = ((dque->tail_cnt == 0 && dque->header_cnt == dque->num_entries - 1)
             || dque->header_cnt == dque->tail_cnt - 1);
   if(flag)
      return  ERR_OVERFLOW;  //--  full
   //-- wr  data
   dque->data_fifo[dque->header_cnt] = data_ptr;
   dque->header_cnt++;
   if(dque->header_cnt >= dque->num_entries)
      dque->header_cnt = 0;

   return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int  dque_fifo_read(_DQUE * dque, void ** data_ptr)
{
   if(dque == NULL || data_ptr == NULL)
      return ERR_WRONG_PARAM;

   if(dque->num_entries <= 0)
      return ERR_OUT_OF_MEM;

   if(dque->tail_cnt == dque->header_cnt)
      return ERR_UNDERFLOW; //-- empty

   //-- rd data
   *data_ptr  =  dque->data_fifo[dque->tail_cnt];
   dque->tail_cnt++;
   if(dque->tail_cnt >= dque->num_entries)
      dque->tail_cnt = 0;

   return ERR_NO_ERR;
}
//-------------------------------------------------------------------------
//-- Structure's field dque->id_dque have to be set to 0
int kern_queue_create(_DQUE *dque,     //-- Ptr to already existing TN_DQUE
                      void **data_fifo,//-- Ptr to already existing array of void * to store data queue entries.Can be NULL
                      int num_entries   //-- Capacity of data queue(num entries).Can be 0
                   )
{
   if(dque == NULL || num_entries < 0 || dque->id_dque == KERN_ID_DATAQUEUE)
      return ERR_WRONG_PARAM;

   queue_reset(&(dque->wait_send_list));
   queue_reset(&(dque->wait_receive_list));

   dque->data_fifo = data_fifo;
   dque->num_entries = num_entries;
   if(dque->data_fifo == NULL)
      dque->num_entries = 0;

   dque->tail_cnt   = 0;
   dque->header_cnt = 0;

   dque->id_dque = KERN_ID_DATAQUEUE;

   return  ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int kern_queue_delete(_DQUE * dque)
{
   KERN_INTSAVE_DATA

   _QUEUE * que;
   _TCB * task;

   if(dque == NULL)
      return ERR_WRONG_PARAM;

   if(dque->id_dque != KERN_ID_DATAQUEUE)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   while(!is_queue_empty(&(dque->wait_send_list)))
   {
      if(kern_chk_irq_disabled() == 0) // int enable
         kern_disable_interrupt();

     //--- delete from sem wait queue
      que = queue_remove_head(&(dque->wait_send_list));
      task = get_task_by_tsk_queue(que);

      if(task_wait_complete(task,FALSE))
      {
         task->task_wait_rc = ERR_DLT;
         kern_enable_interrupt();
         kern_switch_context();
      }
   }

   while(!is_queue_empty(&(dque->wait_receive_list)))
   {
      if(kern_chk_irq_disabled() == 0) // int enable
         kern_disable_interrupt();

     //--- delete from sem wait queue
      que = queue_remove_head(&(dque->wait_receive_list));
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

   dque->id_dque = 0; // Data queue not exists now

   kern_enable_interrupt();

   return ERR_NO_ERR;

}

//----------------------------------------------------------------------------
int kern_queue_send(_DQUE * dque,void *data_ptr,unsigned int timeout)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code
   _QUEUE * que;
   _TCB * task;

   //-- ToDo: validation for input parameter etc.
   if(dque == NULL || timeout == 0)
      return  ERR_WRONG_PARAM;

   if(dque->id_dque != KERN_ID_DATAQUEUE)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

  // if there are already tasks in the data queue's  wait_receive list
   if(!is_queue_empty(&(dque->wait_receive_list)))
   {
      que  = queue_remove_head(&(dque->wait_receive_list));
      task = get_task_by_tsk_queue(que);

      task->data_elem = data_ptr;

      if(task_wait_complete(task,FALSE))
      {
         kern_enable_interrupt();
         kern_switch_context();
         return  ERR_NO_ERR;
      }
      rc = ERR_NO_ERR;
   }
   else  // if there are no tasks in the data queue's  wait_receive list
   {
      rc = dque_fifo_write(dque,data_ptr);
      if(rc != ERR_NO_ERR)  //-- No free entries in data queue
      {
         kern_curr_run_task->data_elem = data_ptr;  //-- Store data_ptr
         task_curr_to_wait_action(&(dque->wait_send_list),
                                         TSK_WAIT_REASON_DQUE_WSEND,timeout);
         kern_enable_interrupt();
         kern_switch_context();
         return kern_curr_run_task->task_wait_rc;
        // return  ERR_NO_ERR;
      }
   }
   kern_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int kern_queue_send_polling(_DQUE * dque,void *data_ptr)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code
   _QUEUE * que;
   _TCB * task;

   //-- ToDo: validation for input parameter etc.
   if(dque == NULL)
      return  ERR_WRONG_PARAM;

   if(dque->id_dque != KERN_ID_DATAQUEUE)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

  // if there are already tasks in the data queue's  wait_receive list
   if(!is_queue_empty(&(dque->wait_receive_list)))
   {
      que  = queue_remove_head(&(dque->wait_receive_list));
      task = get_task_by_tsk_queue(que);

      task->data_elem = data_ptr;

      if(task_wait_complete(task,FALSE))
      {
         kern_enable_interrupt();
         kern_switch_context();
         return  ERR_NO_ERR;
      }
      rc = ERR_NO_ERR;
   }
   else  // if there are no tasks in the data queue's  wait_receive list
   {
      rc = dque_fifo_write(dque,data_ptr);
      if(rc != ERR_NO_ERR)  //-- No free entries in data queue
         rc = ERR_TIMEOUT;  //-- Just convert errorcode
   }

   kern_enable_interrupt();
   return rc;
}

//----------------------------------------------------------------------------
int kern_queue_isend_polling(_DQUE * dque,void *data_ptr)
{
   KERN_INTSAVE_DATA_INT
   int rc; //-- return code
   _QUEUE * que;
   _TCB * task;

   //-- ToDo: validation for input parameter etc.
   if(dque == NULL)
      return  ERR_WRONG_PARAM;

   if(dque->id_dque != KERN_ID_DATAQUEUE)
      return ERR_NOEXS;

   KERN_CHECK_INT_CONTEXT

   kern_idisable_interrupt();

  // if there are already tasks in the data queue's  wait_receive list
   if(!is_queue_empty(&(dque->wait_receive_list)))
   {
      que  = queue_remove_head(&(dque->wait_receive_list));
      task = get_task_by_tsk_queue(que);

      task->data_elem = data_ptr;

      if(task_wait_complete(task,FALSE))
      {
         kern_context_switch_request = 1;
         kern_ienable_interrupt();
         return ERR_NO_ERR;
      }
      rc = ERR_NO_ERR;
   }
   else  // if there are no tasks in the data queue's  wait_receive list
   {
      rc = dque_fifo_write(dque,data_ptr);

      if(rc != ERR_NO_ERR)  //-- No free entries in data queue
         rc = ERR_TIMEOUT;  //-- Just convert errorcode
   }
   kern_ienable_interrupt();
   return rc;
}

//----------------------------------------------------------------------------
int kern_queue_receive(_DQUE *dque,void **data_ptr,unsigned int timeout)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code
   _QUEUE * que;
   _TCB * task;

   //-- ToDo: validation for input parameter etc.
   if(dque == NULL || timeout == 0 || data_ptr == NULL)
      return  ERR_WRONG_PARAM;

   if(dque->id_dque != KERN_ID_DATAQUEUE)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   rc = dque_fifo_read(dque,data_ptr);
   if(rc == ERR_NO_ERR)  //-- There was entry(s) in data queue
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task = get_task_by_tsk_queue(que);

         dque_fifo_write(dque,task->data_elem); //-- Put to data FIFO

         if(task_wait_complete(task,FALSE))
         {
            kern_enable_interrupt();
            kern_switch_context();
            return ERR_NO_ERR;
         }
      }
   }
   else //-- data FIFO is empty
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task = get_task_by_tsk_queue(que);

         *data_ptr = task->data_elem; //-- Return to caller

         if(task_wait_complete(task,FALSE))
         {
            kern_enable_interrupt();
            kern_switch_context();
            return ERR_NO_ERR;
         }
         rc = ERR_NO_ERR;
      }
      else //--   wait_send_list is empty
      {
         task_curr_to_wait_action(&(dque->wait_receive_list),
                                     TSK_WAIT_REASON_DQUE_WRECEIVE,timeout);
         kern_enable_interrupt();
         kern_switch_context();
         //-- When return to this point,in data_elem have to be  valid value
         *data_ptr = kern_curr_run_task->data_elem; //-- Return to caller
         return kern_curr_run_task->task_wait_rc;
        // return  TERR_NO_ERR;
      }
   }
   kern_enable_interrupt();
   return rc;
}

//----------------------------------------------------------------------------
int kern_queue_receive_polling(_DQUE * dque,void **data_ptr)
{
   KERN_INTSAVE_DATA
   int rc; //-- return code
   _QUEUE * que;
   _TCB * task;

   //-- ToDo: validation for input parameter etc.
   if(dque == NULL || data_ptr == NULL)
      return  ERR_WRONG_PARAM;

   if(dque->id_dque != KERN_ID_DATAQUEUE)
      return ERR_NOEXS;

   KERN_CHECK_NON_INT_CONTEXT

   kern_disable_interrupt();

   rc = dque_fifo_read(dque,data_ptr);
   if(rc == ERR_NO_ERR)  //-- There was entry(s) in data queue
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task = get_task_by_tsk_queue(que);

         dque_fifo_write(dque,task->data_elem); //-- Put to data FIFO

         if(task_wait_complete(task,FALSE))
         {
            kern_enable_interrupt();
            kern_switch_context();
            return ERR_NO_ERR;
         }
      }
   }
   else //-- data FIFO is empty
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task = get_task_by_tsk_queue(que);

         *data_ptr = task->data_elem; //-- Return to caller

         if(task_wait_complete(task,FALSE))
         {
            kern_enable_interrupt();
            kern_switch_context();
            return ERR_NO_ERR;
         }
         rc = ERR_NO_ERR;
      }
      else //--   wait_send_list is empty
      {
         rc = ERR_TIMEOUT;
         //find_next_task_to_run();
      }
   }
   kern_enable_interrupt();
   return rc;
}

//----------------------------------------------------------------------------
int kern_queue_ireceive(_DQUE * dque,void ** data_ptr)
{
   KERN_INTSAVE_DATA_INT
   int rc; //-- return code
   _QUEUE * que;
   _TCB * task;

   //-- ToDo: validation for input parameter etc.
   if(dque == NULL || data_ptr == NULL)
      return  ERR_WRONG_PARAM;

   if(dque->id_dque != KERN_ID_DATAQUEUE)
      return ERR_NOEXS;

   KERN_CHECK_INT_CONTEXT

   kern_idisable_interrupt();

   rc = dque_fifo_read(dque,data_ptr);
   if(rc == ERR_NO_ERR)  //-- There was entry(s) in data queue
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task = get_task_by_tsk_queue(que);

         dque_fifo_write(dque,task->data_elem); //-- Put to data FIFO

         if(task_wait_complete(task,FALSE))
         {
            kern_context_switch_request = 1;
            kern_ienable_interrupt();
            return ERR_NO_ERR;
         }
      }
   }
   else //-- data FIFO is empty
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task =  get_task_by_tsk_queue(que);

        *data_ptr = task->data_elem; //-- Return to caller

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
         rc = ERR_TIMEOUT;
      }
   }

   kern_ienable_interrupt();
   return rc;
}
//----------------------------------------------------------------------------
