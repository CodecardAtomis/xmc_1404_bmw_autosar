#include "XMC1400.h"

typedef struct {

  uint32_t      id;      
  uint32_t      dlc;      
  char          data[8];
  
} _CAN_MSG_OBJ;

typedef struct _T_CAN_QUEUE
{
    struct              _T_CAN_QUEUE *prev;
    struct              _T_CAN_QUEUE *next;
    _CAN_MSG_OBJ        *canObj;
    
}_CAN_QUEUE;


#define XMC_CAN_CANCLKSRC_MCLK   0x1U
#define XMC_CAN_DM_NORMAL  1U      /**< Normal divider mode */
#define XMC_CAN_DM_FRACTIONAL  2U  /**< Fractional divider mode */
#define XMC_CAN_DM_OFF  3U          /**< Divider Mode in off-state*/

#define XMC_CAN_MO_MOAR_STDID_Pos (18U)		/**< Standard Identifier bitposition */
#define XMC_CAN_MO_MOAR_STDID_Msk ((0x000007FFUL) << XMC_CAN_MO_MOAR_STDID_Pos) /**< Standard Identifier bitMask */
#define CAN_NODE_NIPR_Msk         (0x7UL)	/**< Node event mask */
#define CAN_MO_MOIPR_Msk          (0x7U)	/**< Message Object event mask */

typedef enum XMC_CAN_PANCMD
{
  XMC_CAN_PANCMD_INIT_LIST = 1U,              /**< Command to initialize a list */
  XMC_CAN_PANCMD_STATIC_ALLOCATE = 2U,        /**< Command to activate static allocation */
  XMC_CAN_PANCMD_DYNAMIC_ALLOCATE = 3U,       /**< Command to activate dynamic allocation */

  XMC_CAN_PANCMD_STATIC_INSERT_BEFORE = 4U,	  /**< Remove a message object from the list and insert it before a given object.*/
  XMC_CAN_PANCMD_DYNAMIC_INSERT_BEFORE = 5U,  /**< Command to activate dynamic allocation */
  XMC_CAN_PANCMD_STATIC_INSERT_BEHIND = 6U,   /**< Command to activate dynamic allocation */
  XMC_CAN_PANCMD_DYNAMIC_INSERT_BEHIND = 7U   /**< Command to activate dynamic allocation */
} XMC_CAN_PANCMD_t;

typedef enum XMC_CAN_MO_TYPE
{
  XMC_CAN_MO_TYPE_RECMSGOBJ,   /**< Receive Message Object selected */
  XMC_CAN_MO_TYPE_TRANSMSGOBJ  /**< Transmit Message Object selected */
} XMC_CAN_MO_TYPE_t;

typedef enum XMC_CAN_NODE_RECEIVE_INPUT
{
  XMC_CAN_NODE_RECEIVE_INPUT_RXDCA, 	/**< CAN Receive Input A */
  XMC_CAN_NODE_RECEIVE_INPUT_RXDCB,		/**< CAN Receive Input B */
  XMC_CAN_NODE_RECEIVE_INPUT_RXDCC,		/**< CAN Receive Input C */
  XMC_CAN_NODE_RECEIVE_INPUT_RXDCD,		/**< CAN Receive Input D */
  XMC_CAN_NODE_RECEIVE_INPUT_RXDCE,		/**< CAN Receive Input E */
  XMC_CAN_NODE_RECEIVE_INPUT_RXDCF,		/**< CAN Receive Input F */
  XMC_CAN_NODE_RECEIVE_INPUT_RXDCG,		/**< CAN Receive Input G */
  XMC_CAN_NODE_RECEIVE_INPUT_RXDCH		/**< CAN Receive Input H */
} XMC_CAN_NODE_RECEIVE_INPUT_t;

#define CAN_NODE0_RXD_P1_0   	XMC_CAN_NODE_RECEIVE_INPUT_RXDCG

#define CAN_NODE1_RXD_P2_10   	XMC_CAN_NODE_RECEIVE_INPUT_RXDCE

//#define CAN_NODE1_RXD_P4_8   	XMC_CAN_NODE_RECEIVE_INPUT_RXDCC
//#define CAN_NODE1_RXD_P4_9   	XMC_CAN_NODE_RECEIVE_INPUT_RXDCD

typedef CAN_GLOBAL_TypeDef      XMC_CAN_t;  
typedef CAN_NODE_TypeDef        XMC_CAN_NODE_t; 

typedef struct {

  XMC_CAN_t      *reg;
  XMC_CAN_NODE_t *node;
  void  (*init)(long );
  void  (*enable)(void);
  void  (*disable)(void);
  void  (*set_speed) (long);
  void  (*disable_mbox)(int);
  void  (*enable_mbox)(int, int);
  void  (*start)(void);
  void  (*stop)(void);
  void  (*configure_mbox)(int mbox, uint32_t id, uint32_t mask, int prior, int mode);
  void  (*configure_ext_mbox)(int mbox, uint32_t id, int prior, int mode);
  int   (*send) (int mbox, char *buff, int dlc, int to);
  int   (*recv) (int mbox, char *buff, uint32_t *_id, int to);

} _CAN;

void can0_init (long speed);
void can0_enable (void);
void can0_disable (void);
void can0_set_speed (long speed);
void can0_disable_mbox (int mbox);
void can0_enable_mbox (int mbox, int mode);
void can0_configure_mbox (int mbox, uint32_t id, uint32_t mask, int prior, int mode);
void can0_configure_ext_mbox (int mbox, uint32_t id, int prior, int mode);
int can0_recv (int mbox, char *buff, uint32_t *_id, int to);
int can0_send (int mbox, char *buff, int dlc, int to);
void can0_open (void);

void can0_enable_configuration_change (void);
void can0_disable_configuration_change (void);

void can1_init (long speed);
void can1_enable (void);
void can1_disable (void);
void can1_set_speed (long speed);
void can1_disable_mbox (int mbox);
void can1_enable_mbox (int mbox, int mode);
void can1_configure_mbox (int mbox, uint32_t id, uint32_t mask, int prior, int mode);
void can1_configure_ext_mbox (int mbox, uint32_t id, int prior, int mode);
int can1_recv (int mbox, char *buff, uint32_t *_id, int to);
int can1_send (int mbox, char *buff, int dlc, int to);
void can1_open (void);

void can1_enable_configuration_change (void);
void can1_disable_configuration_change (void);

void can_queue_reset(_CAN_QUEUE *que);
int can_is_queue_empty(_CAN_QUEUE *que);
int can_queue_size(_CAN_QUEUE *que);
void can_dispose_que(_CAN_QUEUE *que);
void can_queue_add_head(_CAN_QUEUE *que, _CAN_QUEUE *entry);
void can_queue_add_tail(_CAN_QUEUE *que, _CAN_QUEUE *entry);
_CAN_QUEUE *can_queue_remove_head(_CAN_QUEUE *que);
_CAN_QUEUE *can_queue_remove_tail(_CAN_QUEUE *que);
void can_queue_remove_entry(_CAN_QUEUE *entry);
int can_queue_contains_entry(_CAN_QUEUE *que, _CAN_QUEUE *entry);
