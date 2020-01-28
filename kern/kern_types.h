
#ifndef TRUE
#define TRUE      1
#endif

#ifndef FALSE
#define FALSE     0
#endif

#ifndef NULL
#define NULL      0
#endif

// -------- MEMORY MANEGMENT ---------
#define MEMORY_PAGE_SIZE    16
#define DYNAMIC_MEMORY_SIZE 0x2000  // 8KB dinamiskai dalinama atmintis, total SRAM = 16KB
//#define DYNAMIC_MEMORY_START 0x10002000 // + sizeof(os_data)
#define DYNAMIC_MEMORY_START 0x20000000   //16 MB

typedef struct {

  unsigned long size;
  unsigned long addr;

} _memory_unit;


typedef struct {                    // size 0x400

  unsigned char bit_block[DYNAMIC_MEMORY_SIZE / (MEMORY_PAGE_SIZE * 8)];        // 512 * 8 pages  16 bytes
  _memory_unit  pointer[DYNAMIC_MEMORY_SIZE / (MEMORY_PAGE_SIZE * 8)];        // {Pointer, size}

} _dinamic_sram;

void srandom (long seed);
long random (void);
long get_free_mem (void);
long malloc(long sz);
void free(long p);
long kern_get_tickcount (void);
long kern_get_random (void);
void kern_stop_multitasking (void);
void kern_start_multitasking (void);

// --------- CONSTANTS ---------------
#define  KERN_MIN_STACK_SIZE      64

#define  KERN_NUM_PRIORITY        32  //-- 0..31  Priority 0 always is used by timers task

#define  KERN_ST_STATE_NOT_RUN    0
#define  KERN_ST_STATE_RUNNING    1

#define  KERN_WAIT_INFINITE        0xFFFFFFFF
#define  KERN_FILL_STACK_VAL       0xFFFFFFFF

#define  KERN_TASK_START_ON_CREATION     1
#define  KERN_TASK_TIMER              0x80
#define  KERN_TASK_IDLE               0x40

#define  KERN_ID_TASK              0x47ABCF69
#define  KERN_ID_SEMAPHORE         0x6FA173EB
#define  KERN_ID_EVENT             0x5E224F25
#define  KERN_ID_DATAQUEUE         0x8C8A6C89
#define  KERN_ID_FSMEMORYPOOL      0x26B7CE8B
#define  KERN_ID_MUTEX             0x17129E45
#define  KERN_ID_RENDEZVOUS        0x74289EBD

#define  KERN_EXIT_AND_DELETE_TASK       1


// --------- TASK STATES ----------

#define  TSK_STATE_RUNNABLE   0x01
//#define  TSK_STATE_RDY        0x02
#define  TSK_STATE_WAIT       0x04
#define  TSK_STATE_SUSPEND    0x08
#define  TSK_STATE_WAITSUSP   (TSK_STATE_SUSPEND | TSK_STATE_WAIT)
#define  TSK_STATE_DORMANT    0x10

// --------- WAITING ----------

#define  TSK_WAIT_REASON_SLEEP            0x0001
#define  TSK_WAIT_REASON_SEM              0x0002
#define  TSK_WAIT_REASON_EVENT            0x0004
#define  TSK_WAIT_REASON_DQUE_WSEND       0x0008
#define  TSK_WAIT_REASON_DQUE_WRECEIVE    0x0010
#define  TSK_WAIT_REASON_MUTEX_C          0x0020
#define  TSK_WAIT_REASON_MUTEX_C_BLK      0x0040
#define  TSK_WAIT_REASON_MUTEX_I          0x0080
#define  TSK_WAIT_REASON_MUTEX_H          0x0100
#define  TSK_WAIT_REASON_RENDEZVOUS       0x0200
#define  TSK_WAIT_REASON_WFIXMEM          0x2000

#define  KERN_EVENT_ATTR_SINGLE           1
#define  KERN_EVENT_ATTR_MULTI            2
#define  KERN_EVENT_ATTR_CLR              4

#define  KERN_EVENT_WCOND_OR              8
#define  KERN_EVENT_WCOND_AND          0x10

#define  KERN_MUTEX_ATTR_CEILING          1
#define  KERN_MUTEX_ATTR_INHERIT          2

// ------------- ERROR CODES ------------
#define  ERR_NO_ERR           0
#define  ERR_OVERFLOW       (-1)   //-- OOV
#define  ERR_WCONTEXT       (-2)   //-- Wrong context context error
#define  ERR_WSTATE         (-3)   //-- Wrong state   state error
#define  ERR_TIMEOUT        (-4)   //-- Polling failure or timeout
#define  ERR_WRONG_PARAM    (-5)
#define  ERR_UNDERFLOW      (-6)
#define  ERR_OUT_OF_MEM     (-7)
#define  ERR_ILUSE          (-8)   //-- Illegal using
#define  ERR_NOEXS          (-9)   //-- Non-valid or Non-existent object
#define  ERR_DLT           (-10)   //-- Waiting object deleted

#define  KERN_INVALID_VAL     0xFFFFFFFF

#define  NO_TIME_SLICE        0
#define  MAX_TIME_SLICE       0xFFFE
// ---------- CIRCULAR QUEUE -------------

typedef struct _T_QUEUE
{
    struct  _T_QUEUE * prev;
    struct  _T_QUEUE * next;

}_QUEUE;

//------------ TASK ------------

typedef struct _T_TCB
{
    unsigned int *task_sp;      //-- Pointer to task's top of stack

    _QUEUE task_queue;            //-- Queue is used to include task in ready/wait lists
    _QUEUE timer_queue;           //-- Queue is used to include task in timer(timeout,etc.) list
    _QUEUE block_queue;           //-- Queue is used to include task in blocked task list only
                                  // uses for mutexes priority seiling protocol
    _QUEUE create_queue;          //-- Queue is used to include task in create list only
    _QUEUE mutex_queue;           //-- List of all mutexes that tack locked  (ver 2.x)
    _QUEUE *pwait_queue;         //-- Ptr to object's(semaphor,event,etc.) wait list,
                                  // that task has been included for waiting (ver 2.x)
    struct _T_TCB *blk_task;      //-- Store task,that blocked our task(for mutexes's
                                  // priority ceiling protocol only (ver 2.x)

    unsigned int *sp_start;          //-- Base address of task's stack space
    int   sp_size;                //-- Task's stack size (in sizeof(void*),not bytes)
    void *task_func_addr;         //-- filled on creation  (ver 2.x)
    void *task_func_param;        //-- filled on creation  (ver 2.x)

   int  skip_cnt;            //-- Task base priority  (ver 2.x)
   int  priority;                 //-- Task current priority
   int  id_task;                  //-- ID for verification(is it a task or another object?)
                                  // All tasks have the same id_task magic number (ver 2.x)
   int  id;                       // unic id

   int  task_state;               //-- Task state
   int  task_wait_reason;         //-- Reason for waiting
   int  task_wait_rc;             //-- Waiting return code(reason why waiting  finished)
   int  tick_count;               //-- Remaining time until timeout
   int  tslice_count;             //-- Time slice counter

   int  ewait_pattern;            //-- Event wait pattern
   int  ewait_mode;               //-- Event wait mode:  _AND or _OR

   void *data_elem;               //-- Store data queue entry,if data queue is full

   int  activate_count;      //-- Activation request count - for statistic
   int  wakeup_count;        //-- Wakeup request count - for statistic
   int  suspend_count;       //-- Suspension count - for statistic

   // --- used for file system only --
   //long current_sector_read;  // latest sector read by task
   //long buffer_pointer;
   //unsigned long pub_sector_start;
   //unsigned long dos_entry_sector;
   //unsigned long dos_entry_offset;
   //void   *dir_entry;         // dir entry pointer
   //char *sector_buffer;

}_TCB;

//-------- SEMAPHORE --------------

typedef struct _T_SEM
{
   _QUEUE  wait_queue;
   int count;
   int max_count;
   int id_sem;     //-- ID for verification(is it a semaphore or another object?)
                     // All semaphores have the same id_sem magic number (ver 2.x)
}_SEM;

//----- Eventflag --------------

typedef struct _T_EVENT
{
   _QUEUE wait_queue;
   int attr;               //-- Eventflag attribute
   unsigned int pattern;   //-- Initial value of the eventflag bit pattern
   int *io_addr;
   int id_event;           //-- ID for verification(is it a event or another object?)
                             // All events have the same id_event magic number (ver 2.x)
}_EVENT;

//----- DATA QUEUE -------------

typedef struct _T_DQUE
{
   _QUEUE  wait_send_list;
   _QUEUE  wait_receive_list;

   void **data_fifo;        //-- Array of void* to store data queue entries
   int  num_entries;        //-- Capacity of data_fifo(num entries)
   int  tail_cnt;           //-- Counter to processing data queue's Array of void*
   int  header_cnt;         //-- Counter to processing data queue's Array of void*
   int  id_dque;            //-- ID for verification(is it a data queue or another object?)
                            // All data queues have the same id_dque magic number (ver 2.x)
}_DQUE;

//----- Fixed-sized blocks memory pool --------------

typedef struct _T_FMP
{
   _QUEUE wait_queue;

   unsigned int block_size; //-- Actual block size (in bytes)
   int num_blocks;          //-- Capacity (Fixed-sized blocks actual max qty)
   void *start_addr;       //-- Memory pool actual start address
   void *free_list;        //-- Ptr to free block list
   int fblkcnt;             //-- Num of free blocks
   int id_fmp;              //-- ID for verification(is it a fixed-sized blocks memory pool or another object?)
                              // All Fixed-sized blocks memory pool have the same id_fmp magic number (ver 2.x)
}_FMP;


//----- Mutex ------------

typedef struct _T_MUTEX
{
   _QUEUE wait_queue;         //-- List of tasks that wait a mutex
   _QUEUE mutex_queue;        //-- To include in task's locked mutexes list (if any)
   _QUEUE lock_mutex_queue;   //-- To include in system's locked mutexes list
   int attr;                  //-- Mutex creation attr - CEILING or INHERIT

   _TCB *holder;              //-- Current mutex owner(task that locked mutex)
   int ceil_priority;         //-- When mutex created with CEILING attr
   int cnt;                   //-- Reserved
   int id_mutex;              //-- ID for verification(is it a mutex or another object?)
                              // All mutexes have the same id_mutex magic number (ver 2.x)
}_MUTEX;

typedef void (*usb_owner) (void *param);

#define  KERN_INTSAVE_DATA_INT	   int kern_save_status_reg = 0;  //--Cortex-M3
#define  KERN_INTSAVE_DATA         int kern_save_status_reg = 0;

#define  kern_disable_interrupt()  kern_save_status_reg = kern_cpu_save_sr()
#define  kern_enable_interrupt()   kern_cpu_restore_sr(kern_save_status_reg)

   //-- Cortex-M3

#define  kern_idisable_interrupt() kern_save_status_reg = kern_cpu_save_sr()
#define  kern_ienable_interrupt()  kern_cpu_restore_sr(kern_save_status_reg)

int kern_inside_int(void);

#define  KERN_CHECK_INT_CONTEXT   \
             if(!kern_inside_int()) \
                return ERR_WCONTEXT;

#define  KERN_CHECK_INT_CONTEXT_NORETVAL  \
             if(!kern_inside_int())     \
                return;

#define  KERN_CHECK_NON_INT_CONTEXT   \
             if(kern_inside_int()) \
                return ERR_WCONTEXT;

#define  KERN_CHECK_NON_INT_CONTEXT_NORETVAL   \
             if(kern_inside_int()) \
                return ;
