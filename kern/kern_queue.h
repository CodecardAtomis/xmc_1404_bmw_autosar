
void queue_reset(_QUEUE *que);
int is_queue_empty(_QUEUE *que);
void queue_add_head(_QUEUE * que, _QUEUE * entry);
void queue_add_tail(_QUEUE * que, _QUEUE * entry);
_QUEUE * queue_remove_head(_QUEUE * que);
_QUEUE * queue_remove_tail(_QUEUE * que);
void queue_remove_entry(_QUEUE * entry);
int queue_contains_entry(_QUEUE * que, _QUEUE * entry);
int  dque_fifo_write(_DQUE * dque, void * data_ptr);
int  dque_fifo_read(_DQUE * dque, void ** data_ptr);
int kern_queue_create(_DQUE * dque,void ** data_fifo,int num_entries);
int kern_queue_delete(_DQUE * dque);
int kern_queue_send(_DQUE * dque,void * data_ptr,unsigned int timeout);
int kern_queue_send_polling(_DQUE * dque,void * data_ptr);
int kern_queue_isend_polling(_DQUE * dque,void * data_ptr);
int kern_queue_receive(_DQUE * dque,void ** data_ptr,unsigned int timeout);
int kern_queue_receive_polling(_DQUE * dque,void ** data_ptr);
int kern_queue_ireceive(_DQUE * dque,void ** data_ptr);
