
int kern_fmem_create(_FMP * fmp,
                     void * start_addr,
                     unsigned int block_size,
                     int num_blocks);

int kern_fmem_delete(_FMP * fmp);
int kern_fmem_get(_FMP * fmp, void ** p_data, unsigned long timeout);
int kern_fmem_get_polling(_FMP *fmp, void **p_data);
int kern_fmem_get_ipolling(_FMP *fmp, void **p_data);
int kern_fmem_release(_FMP *fmp, void *p_data);
void * fm_get(_FMP * fmp);
int fm_put(_FMP * fmp, void * mem);
