
typedef struct {

  short int     sendId;
  short int     recvId;
  char          numDtc;
  char          protocol;
  char          *vinNumber;
  char          *partNumber;
  uint32_t      *dtcList;
  
} _ECU_OBJ;

typedef struct _T_ECU_QUEUE
{
    struct              _T_ECU_QUEUE *prev;
    struct              _T_ECU_QUEUE *next;
    _ECU_OBJ            *ecuObj;
    
}_ECU_QUEUE;


void ecu_queue_reset(_ECU_QUEUE *que);
int ecu_is_queue_empty(_ECU_QUEUE *que);
int ecu_queue_size(_ECU_QUEUE *que);
int ecu_check_id_exist(_ECU_QUEUE *que, short int _id);
void ecu_dispose_que(_ECU_QUEUE *que);
void ecu_queue_add_head(_ECU_QUEUE *que, _ECU_QUEUE *entry);
void ecu_queue_add_tail(_ECU_QUEUE *que, _ECU_QUEUE *entry);
_ECU_QUEUE *ecu_queue_remove_head(_ECU_QUEUE *que);
_ECU_QUEUE *ecu_queue_remove_tail(_ECU_QUEUE *que);
void ecu_queue_remove_entry(_ECU_QUEUE *entry);
int ecu_queue_contains_entry(_ECU_QUEUE *que, _ECU_QUEUE *entry);

void can_send_single (_CAN *pcan, int mboxId, uint32_t canId, int dlc, char *single);
_CAN_MSG_OBJ *can_recv_single (_CAN *pcan, int mboxId, int to);
int can_kwp_recv_frame (_CAN *pcan, int rxMbox, int txMbox, uint32_t txId, char *frame, int buffSize, int to);
int can_kwp_send_frame (_CAN *pcan, int rxMbox, int txMbox, uint32_t txId, char *frame);
int can_bmw_recv_frame (_CAN *pcan, int rxMbox, char extAddr, int txMbox, uint32_t txId, char *frame, int buffSize, int to);
int can_bmw_send_frame (_CAN *pcan, int rxMbox, char extAddr, int txMbox, uint32_t txId, char *frame, int to);
