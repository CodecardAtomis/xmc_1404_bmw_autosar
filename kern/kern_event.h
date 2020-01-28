
int kern_event_create(_EVENT * evf, int attr, int *io_addr, unsigned int pattern);
int tn_event_delete(_EVENT * evf);

int kern_event_wait(_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern,
                    unsigned int timeout);

int kern_event_wait_polling(_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern);

int kern_event_iwait(_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern);

int kern_event_set(_EVENT * evf, unsigned int pattern);
int kern_event_iset(_EVENT * evf, unsigned int pattern);
int kern_event_clear(_EVENT * evf, unsigned int pattern);
int kern_event_iclear(_EVENT * evf, unsigned int pattern);
int scan_event_waitqueue(_EVENT * evf);
