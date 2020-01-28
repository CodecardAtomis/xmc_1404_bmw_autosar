#include "XMC1400.h"
#include "gpio.h"
#include "scu.h"
#include "uart.h"
#include "can.h"
#include "bignum.h"
#include "string.h"

#include "kern\kern_types.h"
#include "kern\kern_task.h"
#include "kern\kern_event.h"
#include "kern\kern_queue.h"
#include "kern\kern_sem.h"
#include "kern\kern_utils.h"
#include "kern\kern.h"

#define TICKS_PER_SECOND 10
#define TICKS_WAIT 500


#define P0_0            0
#define P0_1            1
#define MODE0            P0_1
#define MODE1            P0_0

// --- LED CONFIG --- //
#define P1_4            4
#define P1_5            5
#define LED0            P1_4
#define LED1            P1_5

// --- CAN ENABLE --- //
#define P0_4    4
#define P0_5    5

#define CAN0_EN    P0_4
#define CAN1_EN    P0_5

//#define CAN1_TXD P4_9 //ALT9
//#define CAN1_RXD P4_8


extern _GPIO_0  *pgpio_0;
extern _GPIO_1  *pgpio_1;
extern _GPIO_2  *pgpio_2;
extern _GPIO_3  *pgpio_3;
extern _GPIO_4  *pgpio_4;
extern _SCU     *pscu;
extern _US0     *pus0;
extern _CAN     *pcan0;
extern _CAN     *pcan1;


char    *ptr;

// --- Bootloader Exported --- //
//BOOTLOADER_STRUCT_t     *bootLoaderStruct;
//INT_VECT_TABLE_t        *intVectTable;
//EXT_VECT_TABLE_t        *extVectTable;

unsigned char CalculateCRC8(unsigned char* message, unsigned long nBytes, unsigned char start, unsigned char poly);
unsigned char Crc_CalculateCRC8(unsigned char* Crc_DataPtr, unsigned long Crc_Length, unsigned char Crc_StartValue8, unsigned char Crc_IsFirstCall );

void SysTick_HandlerFunc (void);
extern void PendSV_Handler (void);

_CAN_QUEUE   can0_msg_list;
_CAN_QUEUE   can1_msg_list;

//_ECU_QUEUE   toyota_ecu_list;

_SEM    can0_list_sem;
_SEM    can1_list_sem;
_SEM    printf_semaphore;
extern int     debug_ready;
// ----------- MAIN TASK DATA ---------------
unsigned int main_task_stack[64];
_TCB  main_task;
void main_task_func(void * par);


// --- SPEED Thread data --- //
unsigned int speed_task_stack[80];
_TCB  speed_task;
void speed_task_func(void * par);

// --- ODO Thread data --- //
unsigned int odo_task_stack[80];
_TCB  odo_task;
void odo_task_func(void * par);

char out_buffer[64];
char in_buffer[64];
char ecuStatusBuff[64];

int ecuTotal;
int ecuFaulty;
int startLEDout;
int gwTaskState;

int main(void)
{
    SysTick_Config(SystemCoreClock / TICKS_PER_SECOND);
    NVIC_SetPriority(SysTick_IRQn,0);

    memset(&main_task,sizeof(_TCB),0);
  
    scu_open();
    gpio_0_open();
    gpio_1_open();
    gpio_2_open();
    gpio_3_open();
    gpio_4_open();
    us0_open();
    can0_open();
    can1_open();   
    
    kern_start_system((_TCB*)&main_task,
	main_task_func,
        31,
        &(main_task_stack[64 - 1]),
        64,
        NULL,
        KERN_TASK_IDLE);	  
  
    while(1);
}
//----------------------------------------------------------------------------
//  void main_task_func (void * par)
//----------------------------------------------------------------------------
void main_task_func (void * par)
{
    
    pcan1->init(500000);
    //pcan0->init(500000);
   
    pcan1->configure_mbox(0,0x123,0x7FF,0,XMC_CAN_MO_TYPE_TRANSMSGOBJ);
    pcan1->configure_mbox(2,0x123,0x7FF,0,XMC_CAN_MO_TYPE_TRANSMSGOBJ);
    
    pcan1->configure_mbox(1,0x222,0x000,0,XMC_CAN_MO_TYPE_RECMSGOBJ);
    pcan1->configure_mbox(3,0x222,0x000,0,XMC_CAN_MO_TYPE_RECMSGOBJ);
    pcan1->start();    
    
    //pcan0->configure_mbox(8,0x321,0x7FF,0,XMC_CAN_MO_TYPE_TRANSMSGOBJ);
    //pcan0->configure_mbox(9,0x222,0x000,0,XMC_CAN_MO_TYPE_RECMSGOBJ);
    //pcan0->start();      
     
  
    startLEDout = 0;
    can_queue_reset((_CAN_QUEUE *)&can0_msg_list);
    can_queue_reset((_CAN_QUEUE *)&can1_msg_list);
    
    memset((char *)&can0_list_sem,sizeof(_SEM),0);
    memset((char *)&can1_list_sem,sizeof(_SEM),0);
    //memset((char *)&printf_semaphore,sizeof(_SEM),0);
    
    kern_sem_create((_SEM *)&can0_list_sem,1,1);
    kern_sem_create((_SEM *)&can1_list_sem,1,1);
    //kern_sem_create((_SEM *)&printf_semaphore,1,1); 
    
  
    memset(&speed_task,sizeof(_TCB),0);
    kern_task_create(&speed_task,speed_task_func,2,&(speed_task_stack[80 - 1]),80,NULL,KERN_TASK_START_ON_CREATION);    

    memset(&odo_task,sizeof(_TCB),0);
    kern_task_create(&odo_task,odo_task_func,2,&(odo_task_stack[80 - 1]),80,NULL,KERN_TASK_START_ON_CREATION);     
     
    pgpio_0->set_output(CAN0_EN);
    pgpio_0->set_pin(CAN0_EN,1);    
    pgpio_0->set_output(CAN1_EN);
    pgpio_0->set_pin(CAN1_EN,1);   
    
    pgpio_0->set_output(MODE0);
    pgpio_0->set_pin(MODE0,1);
    pgpio_0->set_output(MODE1);
    pgpio_0->set_pin(MODE1,1);
    
    while(1)
    {
        kern_task_sleep(500);
        //pgpio_4->set_pin(LED0,0);
        //kern_task_sleep(500);
        //pgpio_4->set_pin(LED0,1);
    }
}
//----------------------------------------------------------------------------
//  void spped_task_func (void * par)
//----------------------------------------------------------------------------

const char speed_frame_counter[] = {0xC0,0xC2,0xC4,0xC6,0xC8,0xCA,0xCC,0xCE,0xC1,0xC3,0xC5,0xC7,0xC9,0xCB,0xCD};

void speed_task_func (void * par)
{
    int led0_state = 0;
    char buffer[8];
    //uint32_t rxId;
    //int dlc;
    //_CAN_MSG_OBJ *tmp_obj;
    //_CAN_QUEUE *tmp_que;
    int direction = 0;
    int cntIndex = 0;
    int speedValue = 0;
    unsigned char crcAutosar;
   
    buffer[1] = 0xC0;  
    
    pgpio_1->set_output(LED0);  // --- red
    pgpio_1->set_pin(LED0,0);       
    
    while (1)
    {
        if ((pcan1->node->NSR & 7) != 0)
        {
          pcan1->node->NCR = 0x41;
          pcan1->node->NSR = 0;
          pcan1->node->NCR = 0;
        }
        
        buffer[1] = speed_frame_counter[cntIndex];
        buffer[2] = (speedValue * 60) & 0x00FF;
        buffer[3] = (speedValue * 60) >> 8;
        buffer[4] = 0x91;
        
        crcAutosar = Crc_CalculateCRC8((unsigned char *)&buffer[1],4,0x23,0);
        buffer[0] = crcAutosar;
        
        cntIndex++;
        if (cntIndex > (sizeof(speed_frame_counter) - 1))
          cntIndex = 0;
        
        pcan1->configure_mbox(0,0x1A1,0x7FF,0,XMC_CAN_MO_TYPE_TRANSMSGOBJ);
        pcan1->send(0,(char *)buffer,5,1);
        
        if (direction == 0)
        {
          speedValue++;
          if (speedValue > 200)
          {
            //speedValue = 0;
            direction = 1;
          }
        }
        else
        {
          speedValue--;
          if (speedValue == 0)
          {
              direction = 0;
          }
        }
        
        kern_task_sleep(50);
    }
}
//----------------------------------------------------------------------------
//  void can1_task_func (void * par)
//----------------------------------------------------------------------------

//#define P2_10   10
//#define P2_11   11

//#define CAN1_TX  P2_11
//#define CAN1_RX  P2_10


void odo_task_func (void * par)
{
    int led1_state = 0;
    char buffer[8];
    uint32_t rxId;
    int dlc;
    _CAN_MSG_OBJ *tmp_obj;
    _CAN_QUEUE *tmp_que;   
    int sndCounter = 0;
    
    pgpio_1->set_output(LED1);  // --- red
    pgpio_1->set_pin(LED1,0);       
    
    while(1)
    {
        //?
    }
    
    /*while (1)
    {
        dlc = pcan1->recv(1,(char *)buffer,&rxId,1);
        if (dlc != 0)
        {
            tmp_obj = (_CAN_MSG_OBJ *)malloc(sizeof(_CAN_MSG_OBJ));
            tmp_obj->dlc = dlc;
            tmp_obj->id = rxId;
            memcpy((char *)tmp_obj->data,(char *)buffer,8);
            
            tmp_que = (_CAN_QUEUE *)malloc(sizeof(_CAN_QUEUE));
            tmp_que->canObj = tmp_obj;
            
            kern_sem_acquire((_SEM *)&can0_list_sem,10);
            can_queue_add_tail((_CAN_QUEUE *)&can0_msg_list,tmp_que);
            kern_sem_signal((_SEM *)&can0_list_sem);
            
            //pcan1->configure_mbox(0,rxId,0x7FF,0,XMC_CAN_MO_TYPE_TRANSMSGOBJ);
            //pcan1->send(0,(char *)buffer,8,100);
            
            led1_state ^= 1;
            pgpio_1->set_pin(LED1,led1_state);
        }
        else 
        {
            //?
        }
        
        kern_sem_acquire((_SEM *)&can1_list_sem,10);
        if (can_is_queue_empty((_CAN_QUEUE *)&can1_msg_list) == FALSE)
        {
            tmp_que = can_queue_remove_head((_CAN_QUEUE *)&can1_msg_list);
            kern_sem_signal((_SEM *)&can1_list_sem);
             
            if ((tmp_que != NULL) && (tmp_que->canObj != NULL))
            {
                tmp_obj = tmp_que->canObj;
                pcan1->configure_mbox(0,tmp_obj->id,0x7FF,0,XMC_CAN_MO_TYPE_TRANSMSGOBJ);
                pcan1->send(0,(char *)tmp_obj->data,tmp_obj->dlc,1);
                
                free((long)tmp_obj);
                free((long)tmp_que);             
            }
        }
        else
            kern_sem_signal((_SEM *)&can1_list_sem);
    }*/
    
}
