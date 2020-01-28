
int kern_task_create(_TCB * task,
                 void (*task_func)(void *param),  //-- task function
                 int priority,                    //-- task priority
                 unsigned int * task_stack_start, //-- task stack first addr in memory (bottom)
                 int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
                 void * param,                    //-- task function parameter
                 int option                       //-- Creation option
                 );

int kern_task_get_id (void);
_TCB *kern_task_find_id (int id);
int kern_task_suspend(_TCB * task);
int kern_task_resume(_TCB * task);
int kern_task_sleep(unsigned int timeout);
int kern_task_wakeup(_TCB * task);
int kern_task_iwakeup(_TCB * task);
int kern_task_activate(_TCB * task);
int kern_task_iactivate(_TCB * task);
int kern_task_release_wait(_TCB * task);
int kern_task_irelease_wait(_TCB * task);
void kern_task_exit(int attr);
int kern_task_terminate(_TCB * task);
int kern_task_delete(_TCB * task);
int kern_task_change_priority(_TCB * task, int new_priority);
void find_next_task_to_run(_TCB *next_task_to_run);
void task_to_non_runnable(_TCB * task);
void task_to_runnable(_TCB * task);
int  task_wait_complete(_TCB * task,int tqueue_remove_enable);
void task_curr_to_wait_action(_QUEUE * wait_que,int wait_reason,unsigned int timeout);
int change_running_task_priority(_TCB * task, int new_priority);
int set_current_priority(_TCB * task, int priority);
void task_set_init_state(_TCB* task);
