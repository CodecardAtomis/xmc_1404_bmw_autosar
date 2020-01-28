
void kern_start_system (_TCB * task,           
                 void (*task_func)(void *param),  //-- task function
                 int priority,                    //-- task priority
                 unsigned int *task_stack_start, //-- task stack first addr in memory (bottom)
                 int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
                 void *param,                    //-- task function parameter
                 int option                       //-- Creation option
                 );
//void kern_timer_task_func(void * par);
void  kern_tick_int_processing (void);
void kern_start_exe (void);
void kern_int_exit(void);
void kern_switch_context(void);
