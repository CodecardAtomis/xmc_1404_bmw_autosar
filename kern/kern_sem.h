
int kern_sem_create(_SEM * sem, int start_value, int max_val);
int kern_sem_delete(_SEM * sem);
int kern_sem_signal(_SEM * sem);
int kern_sem_isignal(_SEM * sem);
int kern_sem_acquire(_SEM * sem, unsigned long timeout);
int kern_sem_acquire_polling(_SEM * sem);
int kern_sem_acquire_ipolling(_SEM * sem);
