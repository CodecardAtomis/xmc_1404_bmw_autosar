
int kern_mutex_create(_MUTEX * mutex,int attribute,int ceil_priority);
int kern_mutex_delete(_MUTEX * mutex);
int kern_mutex_lock(_MUTEX * mutex,unsigned int timeout);
int kern_mutex_lock_polling(_MUTEX * mutex);
int kern_mutex_unlock(_MUTEX * mutex);
int do_unlock_mutex(_MUTEX * mutex);
int find_max_blocked_priority(_MUTEX * mutex,int ref_priority);
int enable_lock_mutex(_TCB * curr_task,_TCB ** blk_task);
int try_lock_mutex(_TCB * task);
void remove_task_from_blocked_list(_TCB * task);
