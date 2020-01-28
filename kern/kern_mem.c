#include "kern_types.h"
#include "kern_queue.h"
#include "kern_task.h"
#include "kern_mutex.h"
#include "kern_mem.h"

#define KERN_ALIG     16;

extern _QUEUE kern_locked_mutexes_list;
extern _QUEUE kern_blocked_tasks_list;

extern _TCB * kern_next_task_to_run;     //-- Task to be run after switch context
extern _TCB * kern_curr_run_task;        //-- Task that is running now

void  kern_switch_context_exit(void);
void  kern_switch_context(void);

unsigned int kern_cpu_save_sr(void);
void  kern_cpu_restore_sr(unsigned int sr);
void  kern_start_exe(void);
int   kern_chk_irq_disabled(void);

_TCB * get_task_by_timer_queque(_QUEUE * que);
_TCB * get_task_by_tsk_queue(_QUEUE * que);
_TCB * get_task_by_block_queque(_QUEUE * que);
_MUTEX * get_mutex_by_mutex_queque(_QUEUE * que);
_MUTEX * get_mutex_by_lock_mutex_queque(_QUEUE * que);
_MUTEX * get_mutex_by_wait_queque(_QUEUE * que);

//----------------------------------------------------------------------------
//  Structure's field fmp->id_id_fmp have to be set to 0
//----------------------------------------------------------------------------
int kern_fmem_create(_FMP *fmp,
                     void *start_addr,
                     unsigned int block_size,
                     int num_blocks)
{
    void ** p_tmp;
    unsigned char * p_block;
    unsigned long i,j;

    if(fmp == NULL)
        return ERR_WRONG_PARAM;

    if(fmp->id_fmp == KERN_ID_FSMEMORYPOOL)
        return ERR_WRONG_PARAM;

    if(start_addr == NULL || num_blocks < 2 || block_size < sizeof(int))
    {
        fmp->fblkcnt = 0;
        fmp->num_blocks = 0;
        fmp->id_fmp = 0;
        fmp->free_list = NULL;
        return ERR_WRONG_PARAM;
    }

    queue_reset(&(fmp->wait_queue));

    //-- Prepare addr/block aligment

    i = ((unsigned long)start_addr + (16 - 1)) & (~(16-1));
    fmp->start_addr  = (void*)i;
    fmp->block_size = (block_size + (16 -1)) & (~(16-1));

    i = (unsigned long)start_addr + block_size * num_blocks;
    j = (unsigned long)fmp->start_addr + fmp->block_size * num_blocks;

    fmp->num_blocks = num_blocks;

    while(j > i)  //-- Get actual num_blocks
    {
        j -= fmp->block_size;
        fmp->num_blocks--;
    }

    if(fmp->num_blocks < 2)
    {
        fmp->fblkcnt    = 0;
        fmp->num_blocks = 0;
        fmp->free_list  = NULL;
        return ERR_WRONG_PARAM;
    }

    //-- Set blocks ptrs for allocation -------

    p_tmp = (void **)fmp->start_addr;
    p_block  = (unsigned char *)fmp->start_addr + fmp->block_size;
    for(i = 0; i < (fmp->num_blocks - 1); i++)
    {
        *p_tmp  = (void *)p_block;  //-- contents of cell = addr of next block
        p_tmp   = (void **)p_block;
        p_block += fmp->block_size;
    }
    *p_tmp = NULL;          //-- Last memory block first cell contents -  NULL

    fmp->free_list = fmp->start_addr;
    fmp->fblkcnt   = fmp->num_blocks;

    fmp->id_fmp = KERN_ID_FSMEMORYPOOL;

    //-----------------------------------------

    return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int kern_fmem_delete(_FMP * fmp)
{
   KERN_INTSAVE_DATA
   _QUEUE * que;
   _TCB * task;


    if(fmp == NULL)
        return ERR_WRONG_PARAM;

    if(fmp->id_fmp != KERN_ID_FSMEMORYPOOL)
        return ERR_NOEXS;


    KERN_CHECK_NON_INT_CONTEXT

    while(!is_queue_empty(&(fmp->wait_queue)))
    {
        kern_disable_interrupt();

      //--- delete from sem wait queue

        que = queue_remove_head(&(fmp->wait_queue));
        task = get_task_by_tsk_queue(que);
        if(task_wait_complete(task,TRUE))
        {
            task->task_wait_rc = ERR_DLT;
            kern_enable_interrupt();
            kern_switch_context();
        }
    }

    if(kern_chk_irq_disabled() == 0)
        kern_disable_interrupt();

    fmp->id_fmp = 0;   //-- Fixed-size memory pool not exists now

    kern_enable_interrupt();

    return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int kern_fmem_get(_FMP * fmp, void ** p_data, unsigned long timeout)
{
    KERN_INTSAVE_DATA
    int rc;
    void * ptr;

    if(fmp == NULL || p_data == NULL || timeout == 0)
        return  ERR_WRONG_PARAM;

    if(fmp->id_fmp != KERN_ID_FSMEMORYPOOL)
        return ERR_NOEXS;

    KERN_CHECK_NON_INT_CONTEXT

    kern_disable_interrupt();

    ptr = fm_get(fmp);
    if(ptr != NULL) //-- Get memory
    {
        *p_data = ptr;
        rc = ERR_NO_ERR;
    }
    else
    {
        task_curr_to_wait_action(&(fmp->wait_queue),
                                     TSK_WAIT_REASON_WFIXMEM, timeout);
        kern_enable_interrupt();
        kern_switch_context();

        //-- When returns to this point, in the 'data_elem' have to be valid value

        *p_data = kern_curr_run_task->data_elem; //-- Return to caller

        return kern_curr_run_task->task_wait_rc;
    }

    kern_enable_interrupt();

    return rc;
}

//----------------------------------------------------------------------------
int kern_fmem_get_polling(_FMP *fmp, void **p_data)
{
    KERN_INTSAVE_DATA
    int rc;
    void * ptr;

    if(fmp == NULL || p_data == NULL)
        return  ERR_WRONG_PARAM;

    if(fmp->id_fmp != KERN_ID_FSMEMORYPOOL)
        return ERR_NOEXS;

    KERN_CHECK_NON_INT_CONTEXT

    kern_disable_interrupt();

    ptr = fm_get(fmp);
    if(ptr != NULL) //-- Get memory
    {
        *p_data = ptr;
        rc = ERR_NO_ERR;
    }
    else
        rc = ERR_TIMEOUT;

    kern_enable_interrupt();

    return rc;
}

//----------------------------------------------------------------------------
int kern_fmem_get_ipolling(_FMP *fmp, void **p_data)
{
    KERN_INTSAVE_DATA_INT
    int rc;
    void * ptr;

    if(fmp == NULL || p_data == NULL)
        return  ERR_WRONG_PARAM;

    if(fmp->id_fmp != KERN_ID_FSMEMORYPOOL)
        return ERR_NOEXS;

    KERN_CHECK_INT_CONTEXT

    kern_idisable_interrupt();

    ptr = fm_get(fmp);
    if(ptr != NULL) //-- Get memory
    {
        *p_data = ptr;
        rc = ERR_NO_ERR;
    }
    else
        rc = ERR_TIMEOUT;

    kern_ienable_interrupt();

    return rc;
}

//----------------------------------------------------------------------------
int kern_fmem_release(_FMP *fmp, void *p_data)
{
    KERN_INTSAVE_DATA

    _QUEUE * que;
    _TCB * task;

    if(fmp == NULL || p_data == NULL)
        return  ERR_WRONG_PARAM;

    if(fmp->id_fmp != KERN_ID_FSMEMORYPOOL)
        return ERR_NOEXS;

    KERN_CHECK_NON_INT_CONTEXT

    kern_disable_interrupt();

    if(!is_queue_empty(&(fmp->wait_queue)))
    {
        que = queue_remove_head(&(fmp->wait_queue));
        task = get_task_by_tsk_queue(que);

        task->data_elem = p_data;

        if(task_wait_complete(task,TRUE))
        {
            kern_enable_interrupt();
            kern_switch_context();

            return ERR_NO_ERR;
        }
    }
    else
        fm_put(fmp,p_data);

    kern_enable_interrupt();

    return  ERR_NO_ERR;
}

//----------------------------------------------------------------------------
int kern_fmem_irelease(_FMP * fmp, void * p_data)
{
    KERN_INTSAVE_DATA_INT

    _QUEUE * que;
    _TCB * task;

    if(fmp == NULL || p_data == NULL)
        return  ERR_WRONG_PARAM;

    if(fmp->id_fmp != KERN_ID_FSMEMORYPOOL)
        return ERR_NOEXS;


    KERN_CHECK_INT_CONTEXT

    kern_idisable_interrupt();

    if(!is_queue_empty(&(fmp->wait_queue)))
    {
        que  = queue_remove_head(&(fmp->wait_queue));
        task = get_task_by_tsk_queue(que);

        task->data_elem = p_data;

        if(task_wait_complete(task,TRUE))
        {
            kern_ienable_interrupt();
            return ERR_NO_ERR;
        }
    }
    else
        fm_put(fmp,p_data);

    kern_ienable_interrupt();

    return ERR_NO_ERR;
}

//----------------------------------------------------------------------------
void * fm_get(_FMP * fmp)
{
   void * p_tmp;

   if(fmp->fblkcnt > 0)
   {
      p_tmp = fmp->free_list;
      fmp->free_list = *(void **)fmp->free_list;   //-- ptr - to new free list
      fmp->fblkcnt--;

      return p_tmp;
   }

   return NULL;
}
//----------------------------------------------------------------------------
int fm_put(_FMP * fmp, void * mem)
{
   if(fmp->fblkcnt < fmp->num_blocks)
   {
      *(void **)mem  = fmp->free_list;   //-- insert block into free block list
      fmp->free_list = mem;
      fmp->fblkcnt++;

      return ERR_NO_ERR;
   }

   return ERR_OVERFLOW;
}



