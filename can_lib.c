
#include "XMC1400.h"
#include "gpio.h"
#include "scu.h"
#include "can.h"
#include "can_lib.h"
#include "string.h"
#include "kern\kern_types.h"
#include "kern\kern.h"

extern _CAN     *pcan0;
extern _CAN     *pcan1;

//------------------------------------------------------------------------------
//   void ecu_queue_reset(_FAT_QUEUE *que)
//------------------------------------------------------------------------------
void ecu_queue_reset(_ECU_QUEUE *que)
{
   que->prev = que->next = que;
}
//------------------------------------------------------------------------------
//  int fat_is_queue_empty(_FAT_QUEUE *que)
//------------------------------------------------------------------------------
int ecu_is_queue_empty(_ECU_QUEUE *que)
{
    if (que == NULL)
        return TRUE;

    if(que->next == que && que->prev == que)
        return TRUE;

    return FALSE;
}
//------------------------------------------------------------------------------
//  int fat_is_queue_empty(_FAT_QUEUE *que)
//------------------------------------------------------------------------------
int ecu_queue_size(_ECU_QUEUE *que)
{
    int ret = 0;
    _ECU_QUEUE *first;
    _ECU_QUEUE *next;

    first = que->next;
    next = first->next;

    while ((first != next) && (next != que))
    {
        ret++;
        next = next->next;
    }

    if ((!ecu_is_queue_empty(que)) && (ret == 0))
        ret++;

    return ret;
}
//------------------------------------------------------------------------------
//  int ecu_check_id_exist(_ECU_QUEUE *que, short int _id)
//------------------------------------------------------------------------------
int ecu_check_id_exist(_ECU_QUEUE *que, short int _id)
{
    _ECU_QUEUE *first;
    _ECU_QUEUE *prev;
    _ECU_OBJ *objEcu;

    if (que == NULL)
        return 0;
    
    if (ecu_is_queue_empty(que) == TRUE)
        return 0;
    
    first = que->next;
    prev = first->prev;

    while ((first != prev) && (first != que))
    {
        objEcu = first->ecuObj;
        if (objEcu != NULL)
        {
            if (objEcu->sendId == _id)
                return 1;
        }
        
        first = first->next;
    }

    return 0;
}
//------------------------------------------------------------------------------
//   void fat_queue_add_head(_FAT_QUEUE *que, _FAT_QUEUE *entry)
//------------------------------------------------------------------------------
void ecu_queue_add_head(_ECU_QUEUE *que, _ECU_QUEUE *entry)
{
   entry->next = que->next;
   entry->prev = que;
   entry->next->prev = entry;
   que->next = entry;
}
//------------------------------------------------------------------------------
//  void fat_queue_add_tail(_FAT_QUEUE *que, _FAT_QUEUE *entry)
//------------------------------------------------------------------------------
void ecu_queue_add_tail(_ECU_QUEUE *que, _ECU_QUEUE *entry)
{
   entry->next = que;
   entry->prev = que->prev;
   entry->prev->next = entry;
   que->prev = entry;
}
//------------------------------------------------------------------------------
//   void fat_dispose_que(_FAT_QUEUE *que)
//------------------------------------------------------------------------------
void ecu_dispose_que(_ECU_QUEUE *que)
{
   _ECU_QUEUE *entry;

   while (!ecu_is_queue_empty(que))
   {
      entry = ecu_queue_remove_tail(que);
      if (entry != NULL)
          free((long)entry);
   }

   if (que != NULL)
      free((long)que);
}
//------------------------------------------------------------------------------
//   _FAT_QUEUE *fat_queue_remove_head(_FAT_QUEUE *que)
//------------------------------------------------------------------------------
_ECU_QUEUE *ecu_queue_remove_head(_ECU_QUEUE *que)
{
   _ECU_QUEUE *entry;

   if(que == NULL || que->next == que)
      return (_ECU_QUEUE *) 0;

   entry = que->next;
   entry->next->prev = que;
   que->next = entry->next;

   return entry;
}
//------------------------------------------------------------------------------
// _FAT_QUEUE * fat_queue_remove_tail(_FAT_QUEUE * que)
//------------------------------------------------------------------------------
_ECU_QUEUE *ecu_queue_remove_tail(_ECU_QUEUE *que)
{
   _ECU_QUEUE *entry;

   if(que->prev == que)
      return (_ECU_QUEUE *)0;

   entry = que->prev;
   entry->prev->next = que;
   que->prev = entry->prev;

   return entry;
}
//------------------------------------------------------------------------------
//   void fat_queue_remove_entry(_FAT_QUEUE * entry)
//------------------------------------------------------------------------------
void ecu_queue_remove_entry(_ECU_QUEUE *entry)
{
   entry->prev->next = entry->next;
   entry->next->prev = entry->prev;
}
//------------------------------------------------------------------------------
//   int fat_queue_contains_entry(_FAT_QUEUE * que, _FAT_QUEUE * entry)
//------------------------------------------------------------------------------
int ecu_queue_contains_entry(_ECU_QUEUE *que, _ECU_QUEUE *entry)
{
   int result = 0;
   _ECU_QUEUE *curr_que;

   curr_que = que->next;
   for(;;) //-- for each task in list
   {
      if(curr_que == entry)
      {
         result = 1; //-- Found
         break;
      }
      if(curr_que->next == que) //-- last
         break;
      else
         curr_que = curr_que->next;
   }
   return result;
}

// ----------------------------------------- //
void can_send_single (_CAN *pcan, int mboxId, uint32_t canId, int dlc, char *single)
{
    if (pcan == NULL)
        return;
  
    pcan->configure_mbox(mboxId,canId,0x7FF,0,XMC_CAN_MO_TYPE_TRANSMSGOBJ);
    pcan->send(mboxId,single,dlc,1);
}
// ----------------------------------------- //
_CAN_MSG_OBJ *can_recv_single (_CAN *pcan, int mboxId, int to)
{
    _CAN_MSG_OBJ *tmp_obj;
    uint32_t dlc;
    uint32_t rxId;
    char buffer[8];
    
    dlc = pcan->recv(mboxId,(char *)buffer,&rxId,to);
    if (dlc != 0) 
    {
        tmp_obj = (_CAN_MSG_OBJ *)malloc(sizeof(_CAN_MSG_OBJ));
        if ((uint32_t)tmp_obj == 0xFFFFFFFF)
        {
            //printf(".");
            return NULL;
        }
    
        tmp_obj->dlc = dlc;
        tmp_obj->id = (rxId & 0x7FF);
        memcpy((char *)tmp_obj->data,(char *)buffer,8); 
        return tmp_obj;
    }

    return NULL;
}
// ----------------------------------------- //
int can_kwp_recv_frame (_CAN *pcan, int rxMbox, int txMbox, uint32_t txId, char *frame, int buffSize, int to)
{
    char single[8];
    int kwp_len;
    int recv_ptr = 7;
    _CAN_MSG_OBJ *rx_obj;

    if ((rx_obj = can_recv_single(pcan,rxMbox,to)) != NULL)
    {
        while ((rx_obj->data[1] == 0x7F) && (rx_obj->data[3] == 0x78))
        {
            free((long)rx_obj);
            if ((rx_obj = can_recv_single(pcan,rxMbox,to)) == NULL)
              return 0;
        }

        if (rx_obj->data[0] != 0x10)
        {
            //kwp_len = single[0];
            memcpy(frame,(char *)rx_obj->data,8);
            free((long)rx_obj);
            return 1;
        }
        else
        {
            kwp_len = rx_obj->data[1] - 6;
            memcpy(frame,(char *)&rx_obj->data[1],7);
            free((long)rx_obj);
            
            single[0] = 0x30;
            single[1] = 0x0F;
            single[2] = 0x00;
            single[3] = 0x00;
            can_send_single(pcan,txMbox,txId,8,(char *)single);

            while (kwp_len > 0)
            {
                if ((rx_obj = can_recv_single(pcan,rxMbox,to)) == NULL)
                    return 0;

                memcpy((char *)&frame[recv_ptr],(char *)&rx_obj->data[1],7);
                free((long)rx_obj);
                
                if ((recv_ptr + 7) <= buffSize)
                    recv_ptr += 7;

                if (kwp_len > 7)
                    kwp_len -= 7;
                else
                    kwp_len = 0;

                if (single[0] == 0x2F)
                {
                    single[0] = 0x30;
                    single[1] = 0x0F;
                    single[2] = 0x00;
                    single[3] = 0x00;
                    can_send_single(pcan,txMbox,txId,8,(char *)single);
                }
            }

            return 1;
        }
    }

    return 0;
}
// ----------------------------------------- //
int can_kwp_send_frame (_CAN *pcan, int rxMbox, int txMbox, uint32_t txId, char *frame)
{
    _CAN_MSG_OBJ *rx_obj;
    int window = 0x0F;
    char single[8];
    int kwp_len = frame[0] + 1;
    int send_ptr = 7;
    char send_ack = 0x21;

    if (kwp_len < 8)
    {
        memcpy((char *)single, frame, 8);
        can_send_single(pcan,txMbox,txId,8,(char *)single);
    }
    else
    {
        single[0] = 0x10;
        memcpy((char *)&single[1],frame,7);
        can_send_single(pcan,txMbox,txId,8,(char *)single);
        kwp_len -= 7;

        // --- recv flow controll --- //
        if ((rx_obj = can_recv_single(pcan,rxMbox,1500)) != NULL)
            return 0;

        if ((rx_obj->data[0] & 0x30) == 0x30)
            window = rx_obj->data[1];
        
        free((long)rx_obj);
        
        if (window == 0)
            window = 0x0F;
        
        while (kwp_len > 0)
        {
            single[0] = send_ack;
            memcpy((char *)&single[1],(char *)&frame[send_ptr],7);
            can_send_single(pcan,txMbox,txId,8,(char *)single);

            send_ptr += 7;
            send_ack++;

            if (send_ack > 0x2F)
                send_ack = 0x20;
            
            window--;
            if (window == 0)
            {
                // --- recv flow controll --- //
                if ((rx_obj = can_recv_single(pcan,rxMbox,1500)) != NULL)
                    return 0;

                if ((rx_obj->data[0] & 0x30) == 0x30)
                    window = rx_obj->data[1];
        
                free((long)rx_obj);
        
                if (window == 0)
                    window = 0x0F;
            }
            
            if (kwp_len > 7)
                kwp_len -= 7;
            else
                kwp_len = 0;
        }
    }
    
    return 1;
}
// ----------------------------------------- //
int can_bmw_recv_frame (_CAN *pcan, int rxMbox, char extAddr, int txMbox, uint32_t txId, char *frame, int buffSize, int to)
{
    char single[8];
    int kwp_len;
    int recv_ptr;
    char flc;
    _CAN_MSG_OBJ *rx_obj;

    if ((rx_obj = can_recv_single(pcan,rxMbox,to)) != NULL)
    {
        while ((rx_obj->data[2] == 0x7F) && (rx_obj->data[4] == 0x78))
        {
            free((long)rx_obj);
            if ((rx_obj = can_recv_single(pcan,rxMbox,to)) == NULL)
              return 0;
        }

        if ((rx_obj->data[1] & 0x10) != 0x10)
        {
            //kwp_len = single[0];
            memcpy(frame,(char *)&rx_obj->data[1],7);
            free((long)rx_obj);
            return 1;
        }
        else
        {
            kwp_len = rx_obj->data[2] - 5;
            memcpy(frame,(char *)&rx_obj->data[2],6);
            free((long)rx_obj);
            recv_ptr = 6;
            
            single[0] = extAddr;
            single[1] = 0x30;
            single[2] = 0x0F;
            single[3] = 0x00;
            single[4] = 0x00;
            can_send_single(pcan,txMbox,txId,8,(char *)single);
            
            while (kwp_len > 0)
            {
                if ((rx_obj = can_recv_single(pcan,rxMbox,to)) == NULL)
                {
                    printf("cto\n");
                    return 0;
                }

                if ((recv_ptr + 6) <= buffSize)
                {
                    memcpy((char *)&frame[recv_ptr],(char *)&rx_obj->data[2],6);
                    flc = rx_obj->data[1];
                    recv_ptr += 6;
                }
                
                free((long)rx_obj);
                
                //if ((recv_ptr + 6) <= buffSize)
                //    recv_ptr += 6;

                if (kwp_len > 6)
                    kwp_len -= 6;
                else
                    kwp_len = 0;

                if (flc == 0x2F)
                {
                    single[0] = extAddr;
                    single[1] = 0x30;
                    single[2] = 0x0F;
                    single[3] = 0x00;
                    single[4] = 0x00;
                    can_send_single(pcan,txMbox,txId,8,(char *)single);
                }
            }

            return 1;
        }
    }

    return 0;
}
// ----------------------------------------- //
int can_bmw_send_frame (_CAN *pcan, int rxMbox, char extAddr, int txMbox, uint32_t txId, char *frame, int to)
{
    _CAN_MSG_OBJ *rx_obj;
    int window = 0x0F;  
    char single[8];
    int kwp_len = frame[0] + 2;
    int send_ptr = 6;
    char send_ack = 0x21;

    if (kwp_len < 7)
    {
        memcpy((char *)&single[1], frame, 7);
        single[0] = extAddr;
        //can_send_single(pcan,txMbox,txId,kwp_len,(char *)single);
        can_send_single(pcan,txMbox,txId,8,(char *)single);
    }
    else
    {
        single[0] = extAddr;
        single[1] = 0x10;
        memcpy((char *)&single[2],frame,6);
        can_send_single(pcan,txMbox,txId,8,(char *)single);
        kwp_len -= 6;

        // --- recv flow controll --- //
        if ((rx_obj = can_recv_single(pcan,rxMbox,1500)) != NULL)
            return 0;

        if ((rx_obj->data[1] & 0x30) == 0x30)
            window = rx_obj->data[2];
        
        free((long)rx_obj);
        
        if (window == 0)
            window = 0x0F;        
        
        while (kwp_len > 0)
        {
            single[0] = extAddr;
            single[1] = send_ack;
            memcpy((char *)&single[2],(char *)&frame[send_ptr],6);
            can_send_single(pcan,txMbox,txId,8,(char *)single);

            send_ptr += 6;
            send_ack++;

            if (send_ack > 0x2F)
                send_ack = 0x20;
            
            window--;
            if (window == 0)
            {
                // --- recv flow controll --- //
                if ((rx_obj = can_recv_single(pcan,rxMbox,1500)) != NULL)
                    return 0;

                if ((rx_obj->data[1] & 0x30) == 0x30)
                    window = rx_obj->data[2];
        
                free((long)rx_obj);
        
                if (window == 0)
                    window = 0x0F;
            }            

            if (kwp_len > 6)
                kwp_len -= 6;
            else
                kwp_len = 0;
        }
    }
    
    return 1;
}
