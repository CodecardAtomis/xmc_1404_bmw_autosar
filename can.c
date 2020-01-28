#include "XMC1400.h"
#include "gpio.h"
#include "scu.h"
#include "can.h"

#include "kern\kern_types.h"
#include "kern\kern.h"


#define P1_0    0
#define P1_1    1
#define P2_10   10
#define P2_11   11

#define CAN0_TX  P1_1
#define CAN0_RX  P1_0
#define CAN1_TX  P2_11
#define CAN1_RX  P2_10


/*
#define P1_0    0
#define P1_1    1
#define P4_8    8
#define P4_9    9

#define CAN0_TX  P1_1
#define CAN0_RX  P1_0
#define CAN1_TX  P4_9
#define CAN1_RX  P4_8
*/

extern _GPIO_0    *pgpio_0;
extern _GPIO_1    *pgpio_1;
extern _GPIO_2    *pgpio_2;
extern _GPIO_4    *pgpio_4;
extern _SCU       *pscu;

//----------------------------------------
_CAN  *pcan0;
_CAN   pcan0_buffer;

_CAN  *pcan1;
_CAN   pcan1_buffer;
//----------------------------------------

//------------------------------------------------------------------------------
//   void fat_queue_reset(_FAT_QUEUE *que)
//------------------------------------------------------------------------------
void can_queue_reset(_CAN_QUEUE *que)
{
   que->prev = que->next = que;
}
//------------------------------------------------------------------------------
//  int fat_is_queue_empty(_FAT_QUEUE *que)
//------------------------------------------------------------------------------
int can_is_queue_empty(_CAN_QUEUE *que)
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
int can_queue_size(_CAN_QUEUE *que)
{
    int ret = 0;
    _CAN_QUEUE *first;
    _CAN_QUEUE *next;

    first = que->next;
    next = first->next;

    while ((first != next) && (next != que))
    {
        ret++;
        next = next->next;
    }

    if ((!can_is_queue_empty(que)) && (ret == 0))
        ret++;

    return ret;
}
//------------------------------------------------------------------------------
//   void fat_queue_add_head(_FAT_QUEUE *que, _FAT_QUEUE *entry)
//------------------------------------------------------------------------------
void can_queue_add_head(_CAN_QUEUE *que, _CAN_QUEUE *entry)
{
   entry->next = que->next;
   entry->prev = que;
   entry->next->prev = entry;
   que->next = entry;
}
//------------------------------------------------------------------------------
//  void fat_queue_add_tail(_FAT_QUEUE *que, _FAT_QUEUE *entry)
//------------------------------------------------------------------------------
void can_queue_add_tail(_CAN_QUEUE *que, _CAN_QUEUE *entry)
{
   entry->next = que;
   entry->prev = que->prev;
   entry->prev->next = entry;
   que->prev = entry;
}
//------------------------------------------------------------------------------
//   void fat_dispose_que(_FAT_QUEUE *que)
//------------------------------------------------------------------------------
void can_dispose_que(_CAN_QUEUE *que)
{
   _CAN_QUEUE *entry;

   while (!can_is_queue_empty(que))
   {
      entry = can_queue_remove_tail(que);
      if (entry != NULL)
          free((long)entry);
   }

   if (que != NULL)
      free((long)que);
}
//------------------------------------------------------------------------------
//   _FAT_QUEUE *fat_queue_remove_head(_FAT_QUEUE *que)
//------------------------------------------------------------------------------
_CAN_QUEUE *can_queue_remove_head(_CAN_QUEUE *que)
{
   _CAN_QUEUE *entry;

   if(que == NULL || que->next == que)
      return (_CAN_QUEUE *) 0;

   entry = que->next;
   entry->next->prev = que;
   que->next = entry->next;

   return entry;
}
//------------------------------------------------------------------------------
// _FAT_QUEUE * fat_queue_remove_tail(_FAT_QUEUE * que)
//------------------------------------------------------------------------------
_CAN_QUEUE *can_queue_remove_tail(_CAN_QUEUE *que)
{
   _CAN_QUEUE *entry;

   if(que->prev == que)
      return (_CAN_QUEUE *) 0;

   entry = que->prev;
   entry->prev->next = que;
   que->prev = entry->prev;

   return entry;
}
//------------------------------------------------------------------------------
//   void fat_queue_remove_entry(_FAT_QUEUE * entry)
//------------------------------------------------------------------------------
void can_queue_remove_entry(_CAN_QUEUE *entry)
{
   entry->prev->next = entry->next;
   entry->next->prev = entry->prev;
}
//------------------------------------------------------------------------------
//   int fat_queue_contains_entry(_FAT_QUEUE * que, _FAT_QUEUE * entry)
//------------------------------------------------------------------------------
int can_queue_contains_entry(_CAN_QUEUE *que, _CAN_QUEUE *entry)
{
   int result = 0;
   _CAN_QUEUE *curr_que;

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

//-------------------------------------
//              CAN0
//-------------------------------------
void can0_enable (void)
{
  pscu->enable_peripheral_clock(SCU_CLK_CGATCLR0_MCAN0_Msk);
  //XMC_CAN_Init(CAN, XMC_CAN_CANCLKSRC_MCLK, 11000000); 
  // --- enable module controll --- //
  pcan0->reg->CLC = 2;
  pcan0->reg->MCR = (pcan0->reg->MCR & ~CAN_MCR_CLKSEL_Msk) | XMC_CAN_CANCLKSRC_MCLK ;
}
//-------------------------------------
void can0_disable (void)
{
    pscu->disable_peripheral_clock(SCU_CLK_CGATCLR0_MCAN0_Msk);
}

void can0_enable_configuration_change (void)
{
  pcan0->node->NCR |= (uint32_t)CAN_NODE_NCR_CCE_Msk;
}

void can0_disable_configuration_change (void)
{
  pcan0->node->NCR &= ~(uint32_t)CAN_NODE_NCR_CCE_Msk;
}

void can0_set_init_bit (void)
{
  pcan0->node->NCR |= (uint32_t)CAN_NODE_NCR_INIT_Msk;
}

void can0_reset_init_bit (void)
{
  pcan0->node->NCR &= ~(uint32_t)CAN_NODE_NCR_INIT_Msk;
}
//-------------------------------------
void can0_set_speed (long speed)
{
    uint32_t temp_brp = 12U ;
    uint32_t temp_tseg1 = 12U;
    uint32_t best_brp = 0U;
    uint32_t best_tseg1 = 1U;
    uint32_t best_tseg2 = 0U;
    uint32_t best_tbaud = 0U;
    uint32_t best_error = 10000U;
    
    //uint32_t can_frequency = 11000000;
    uint32_t can_frequency = 11000000;
    
    uint32_t baudrate = speed;
    uint32_t sample_point = 6000;
    uint32_t sjw = 3;    
    
  /*
   * Bit timing & sampling
   * Tq = (BRP+1)/Fcan if DIV8 = 0
   * Tq = 8*(BRP+1)/Fcan if DIV8 = 1
   * TSync = 1.Tq
   * TSeg1 = (TSEG1+1)*Tq                >= 3Tq
   * TSeg2 = (TSEG2+1)*Tq                >= 2Tq
   * Bit Time = TSync + TSeg1 + TSeg2    >= 8Tq
   *
   * Resynchronization:
   *
   * Tsjw = (SJW + 1)*Tq
   * TSeg1 >= Tsjw + Tprop
   * TSeg2 >= Tsjw
   */

    for (temp_brp = 1U; temp_brp <= 64U; temp_brp++)
    {
        uint32_t f_quanta = (uint32_t)((can_frequency * 10U) / temp_brp);
        uint32_t temp_tbaud = (uint32_t)(f_quanta / (baudrate));
        uint32_t temp_baudrate;
        uint32_t error;

        if((temp_tbaud % 10U) > 5U)
        {
            temp_tbaud = (uint32_t)(temp_tbaud / 10U);
            temp_tbaud++;
        }
        else
        {
            temp_tbaud = (uint32_t)(temp_tbaud / 10U);
        }

        if(temp_tbaud > 0U)
        {
            temp_baudrate = (uint32_t) (f_quanta / (temp_tbaud * 10U));
        }
        else
        {
            temp_baudrate = f_quanta / 10U;
            temp_tbaud = 1;
        }

        if(temp_baudrate >= baudrate)
        {
            error = temp_baudrate - baudrate;
        }
        else
        {
            error = baudrate - temp_baudrate;
        }

        if ((temp_tbaud <= 20U) && (best_error > error))
        {
            best_brp = temp_brp;
            best_tbaud = temp_tbaud;
            best_error = (error);

            if (error < 1000U)
                break;
        }
    }
    
    /* search for best sample point */
    best_error = 10000U;

    for (temp_tseg1 = 64U; temp_tseg1 >= 3U; temp_tseg1--)
    {
        uint32_t tempSamplePoint = ((temp_tseg1 + 1U) * 10000U) / best_tbaud;
        uint32_t error;
        
        if (tempSamplePoint >= sample_point)
            error = tempSamplePoint  - sample_point;
        else
            error = sample_point  - tempSamplePoint;

        if (best_error > error)
        {
            best_tseg1 = temp_tseg1;
            best_error = error;
        }
        
        if (tempSamplePoint < (sample_point))
            break;
    }    
   
    best_tseg2 = best_tbaud - best_tseg1 - 1U;

    can0_enable_configuration_change();
    /* Configure bit timing register */
    pcan0->node->NBTR = (((uint32_t)(best_tseg2 - 1u) << CAN_NODE_NBTR_TSEG2_Pos) & (uint32_t)CAN_NODE_NBTR_TSEG2_Msk) |
                   ((((uint32_t)((uint32_t)(sjw)-1U) << CAN_NODE_NBTR_SJW_Pos)) & (uint32_t)CAN_NODE_NBTR_SJW_Msk)|
                   (((uint32_t)(best_tseg1-1U) << CAN_NODE_NBTR_TSEG1_Pos) & (uint32_t)CAN_NODE_NBTR_TSEG1_Msk)|
                   (((uint32_t)(best_brp - 1U) << CAN_NODE_NBTR_BRP_Pos) & (uint32_t)CAN_NODE_NBTR_BRP_Msk)|
                   (((uint32_t)0U << CAN_NODE_NBTR_DIV8_Pos) & (uint32_t)CAN_NODE_NBTR_DIV8_Msk);
    can0_disable_configuration_change();    
}
//-------------------------------------
void can0_setup_clock (void)
{
  //int tq;       // number of sample points per bit
  //int prop;     // signal delivery and return time in tq (bus driver delay + receiver delay = 30ns + bus signal travel 1m ~ 5ns)
  //int BRP;
  //int PHASE1,PHASE2;
  //int SJW;
  
  uint32_t peripheral_frequency;
  uint32_t can_frequency = 11000000;
  uint32_t freq_n, freq_f;
  uint32_t step;
  int normal_divider;
  int can_divider_mode;
  uint32_t step_n, step_f;
  uint32_t can_frequency_khz;
  uint32_t peripheral_frequency_khz;
  
  
  peripheral_frequency = ((pcan0->reg->MCR & CAN_MCR_CLKSEL_Msk) >> CAN_MCR_CLKSEL_Pos);
  
  /* Normal divider mode */
  if ((peripheral_frequency / can_frequency) > 1024)
      step_n = 0;
  else
      step_n = 1024U - (peripheral_frequency / can_frequency);
  
  //step_n = (uint32_t)min(0, 1023U);
  freq_n = (uint32_t) (peripheral_frequency / (1024U - step_n));
  
  /* Fractional divider mode */
  can_frequency_khz = (uint32_t) (can_frequency >> 6);
  peripheral_frequency_khz = (uint32_t)(peripheral_frequency >> 6);

  if (((1024U * can_frequency_khz) / peripheral_frequency_khz) < 1023)
      step_f = (1024U * can_frequency_khz) / peripheral_frequency_khz;
  else
      step_f = 1023; 
  
  //step_f = (uint32_t)(min( (((1024U * can_frequency_khz) / peripheral_frequency_khz) ), 1023U ));
  freq_f = (uint32_t)((peripheral_frequency_khz * step_f) / 1024U);
  freq_f = freq_f << 6;

  if ((uint32_t)(can_frequency - freq_n) <= (can_frequency - freq_f))
      normal_divider = 1;
  else
      normal_divider = 0;
  
  step   = (normal_divider != 0U) ? step_n : step_f;
  can_divider_mode = (normal_divider != 0U) ? XMC_CAN_DM_NORMAL : XMC_CAN_DM_FRACTIONAL;

  pcan0->reg->FDR &= (uint32_t) ~(CAN_FDR_DM_Msk | CAN_FDR_STEP_Msk);
  //pcan1->reg->FDR |= ((uint32_t)can_divider_mode << CAN_FDR_DM_Pos) | ((uint32_t)step << CAN_FDR_STEP_Pos);  
  pcan0->reg->FDR = 0x80EA;
}
//-------------------------------------
void can0_disable_mbox (int mbox)
{
    //pcan1->reg->CAN_MB[mbox].CAN_MMR &= 0xf7ffffff;
}
//-------------------------------------
void can0_enable_mbox (int mbox, int mode)
{
    //pcan1->reg->CAN_MB[mbox].CAN_MMR |= (mode << 24);
}
//-------------------------------------
void can0_configure_mbox (int mbox, uint32_t id, uint32_t mask, int prior, int mode)
{
    //uint32_t reg;
    CAN_MO_TypeDef *can_mbox;
    
    if (mbox > 31)
        return;
    
    can_mbox = (CAN_MO_TypeDef *)&CAN_MO->MO[mbox];    
    can_mbox->MOIPR = (((uint32_t)(mbox / 32) << (CAN_MO_MOIPR_MPN_Pos + 5U)) | ((uint32_t)(mbox % 32) << CAN_MO_MOIPR_MPN_Pos));

    /* Disable Message object */
    can_mbox->MOCTR = CAN_MO_MOCTR_RESMSGVAL_Msk;

    id &= 0x7FF;
    //id &= (uint32_t) ~(CAN_MO_MOAR_ID_Msk);
    can_mbox->MOAR = (1 << 30) | (id << XMC_CAN_MO_MOAR_STDID_Pos);  //0x83fc0000;
    can_mbox->MOAMR = (1 << 29) | (mask << XMC_CAN_MO_MOAR_STDID_Pos); //0x23fc0000
     
    /* Check whether message object is transmit message object */
    if (mode == XMC_CAN_MO_TYPE_TRANSMSGOBJ)
    {
        /* Set MO as Transmit message object  */
        //XMC_CAN_MO_UpdateData(can_mo);
        can_mbox->MOCTR = CAN_MO_MOCTR_SETDIR_Msk;
    }
    else
    {
        /* Set MO as Receive message object and set RXEN bit */
        can_mbox->MOCTR = CAN_MO_MOCTR_RESDIR_Msk;
    }

    /* Reset RTSEL and Set MSGVAL ,TXEN0 and TXEN1 bits */
    can_mbox->MOCTR = (CAN_MO_MOCTR_SETTXEN0_Msk | CAN_MO_MOCTR_SETTXEN1_Msk | CAN_MO_MOCTR_SETMSGVAL_Msk |
                                 CAN_MO_MOCTR_SETRXEN_Msk | CAN_MO_MOCTR_RESRTSEL_Msk);
    
    
    /* allocation of MO to node list */
    pcan0->reg->PANCTR = (((uint32_t)XMC_CAN_PANCMD_STATIC_ALLOCATE << CAN_PANCTR_PANCMD_Pos) & (uint32_t)CAN_PANCTR_PANCMD_Msk) |
		        (((uint32_t)mbox << CAN_PANCTR_PANAR1_Pos) & (uint32_t)CAN_PANCTR_PANAR1_Msk) |
		        (((uint32_t)(1 + 0) << CAN_PANCTR_PANAR2_Pos) & (uint32_t)CAN_PANCTR_PANAR2_Msk);
  
    // uses list 1
    
    //XMC_CAN_PanelControl(obj, XMC_CAN_PANCMD_STATIC_ALLOCATE,mbox,(1 + 1U));
    /* wait until panel as done the command */
    while (pcan0->reg->PANCTR & CAN_PANCTR_BUSY_Msk);    
}
//-------------------------------------
void can0_configure_ext_mbox (int mbox, uint32_t id, int prior, int mode)
{
  /*
    pcan1->reg->CAN_MB[mbox].CAN_MMR = mode + (prior << 16);
    pcan1->reg->CAN_MB[mbox].CAN_MID = (id & 0x1fffffff) | (1 << 29);

    if ((mode == CAN_MMR_MOT_MB_RX) || (mode == CAN_MMR_MOT_MB_RX_OVERWRITE))
    {
        pcan1->reg->CAN_MB[mbox].CAN_MAM = 0x1fffffff;
        pcan1->reg->CAN_MB[mbox].CAN_MCR |= CAN_MCR_MTCR;   // allow receive
    }
*/
}
//-------------------------------------
int can0_mbox_rx_complete (int mbox)
{
    CAN_MO_TypeDef *can_mbox;
    
    if (mbox > 31)
        return 0;  
    
    can_mbox = (CAN_MO_TypeDef *)&CAN_MO->MO[mbox];
    
    if (can_mbox->MOSTAT & CAN_MO_MOSTAT_NEWDAT_Msk)
        return 1;
    
    return 0;
}
//-------------------------------------
int can0_recv (int mbox, char *buff, uint32_t *_id, int to)
{
    CAN_MO_TypeDef *can_mbox;
    uint32_t *plong = (uint32_t *)buff;
    int timeout = kern_get_tickcount() + to;
    int dlc;
    
    can_mbox = (CAN_MO_TypeDef *)&CAN_MO->MO[mbox];
    
    while (can0_mbox_rx_complete(mbox) == 0)
    {
        if (timeout <= kern_get_tickcount())
        {
            return 0;
        }
    }
    
    // --- clear NEWDAT
    plong[0] = can_mbox->MODATAL;
    plong[1] = can_mbox->MODATAH;
    
    // --- clear NEWDAT and RXUPD
    
    can_mbox->MOCTR = (CAN_MO_MOSTAT_NEWDAT_Msk | CAN_MO_MOSTAT_RXUPD_Msk);
    can_mbox->MOCTR = CAN_MO_MOCTR_SETRXEN_Msk;
    
    // --- for EXTENDED --- //
    //*_id = (can_mbox->MOAR >> 0) & 0x1FFFFFFF;  // --- received ID
    
    *_id = (can_mbox->MOAR >> 18) & 0x1FFFFFFF;  // --- received ID
    dlc = (can_mbox->MOFCR >> CAN_MO_MOFCR_DLC_Pos);
    dlc &= 0x0F;

    return dlc;
}
//-------------------------------------
int can0_send (int mbox, char *buff, int dlc, int to)
{
    uint32_t *plong = (uint32_t *)buff;
    int timeout = kern_get_tickcount() + to;
    CAN_MO_TypeDef *can_mbox;
    
    if (mbox > 31)
        return 0;
    
    can_mbox = (CAN_MO_TypeDef *)&CAN_MO->MO[mbox];  

    can_mbox->MOCTR = CAN_MO_MOCTR_RESMSGVAL_Msk;
    /* Configure data length */
    can_mbox->MOFCR = ((can_mbox->MOFCR) & ~(uint32_t)(CAN_MO_MOFCR_DLC_Msk)) |
                                (((uint32_t) dlc << CAN_MO_MOFCR_DLC_Pos) & (uint32_t)CAN_MO_MOFCR_DLC_Msk);
    /* Configure Data registers*/
    can_mbox->MODATAL = plong[0];
    can_mbox->MODATAH = plong[1];
    /* Reset RTSEL and Set MSGVAL ,TXEN0 and TXEN1 bits */
    can_mbox->MOCTR = (CAN_MO_MOCTR_SETNEWDAT_Msk| CAN_MO_MOCTR_SETMSGVAL_Msk |CAN_MO_MOCTR_RESRTSEL_Msk);    
    
    // --- transmit message --- //
    uint32_t mo_type = (uint32_t)(((can_mbox->MOSTAT) & CAN_MO_MOSTAT_MSGVAL_Msk) >> CAN_MO_MOSTAT_MSGVAL_Pos);
    uint32_t mo_transmission_ongoing = (uint32_t) ((can_mbox->MOSTAT) & CAN_MO_MOSTAT_TXRQ_Msk) >> CAN_MO_MOSTAT_TXRQ_Pos;
    
    /* check if message is disabled */
    if (mo_type == 0U)
        return 0;
    else if (mo_transmission_ongoing == 1U)
        return 0;
    else
    {
        /* set TXRQ bit */
        can_mbox-> MOCTR = CAN_MO_MOCTR_SETTXRQ_Msk | CAN_MO_MOCTR_SETTXEN0_Msk | CAN_MO_MOCTR_SETTXEN1_Msk;
    }
    
    return 1;
}
//-------------------------------------
void can0_init (long speed)
{
    int i;

    pcan0->enable();
    can0_setup_clock();
  
    // --- configure CAN0 pins --- //
    pgpio_1->set_input(CAN0_RX);
    //pus0->reg->DXCR[XMC_UART_CH_INPUT_RXD] &= ~(USIC_CH_DX0CR_DSEL_Msk);  
  
    /* Set mode */
    pgpio_1->reg->IOCR[CAN0_TX >> 2U] &= ~(0xFC << 8);
    pgpio_1->reg->IOCR[CAN0_TX >> 2U] |= (uint32_t)XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT9 << (PORT_IOCR_PC_Size * (CAN0_TX & 0x3U));  
  
    //XMC_GPIO_Init(CAN1_TXD, &CAN1_TXD_config);
    //XMC_GPIO_Init(CAN1_RXD, &CAN1_RXD_config);    
    
    can0_enable_configuration_change();

    // --- disable Loop Back mode --- //
    pcan0->node->NPCR = (pcan0->node->NPCR) & ~((uint32_t)CAN_NODE_NPCR_LBM_Msk << CAN_NODE_NPCR_LBM_Pos);
    
    //pcan0->node->NPCR = ((pcan0->node->NPCR) & ~(uint32_t)(CAN_NODE_NPCR_RXSEL_Msk)) | 
    //    (((uint32_t)CAN_NODE0_RXD_P1_0 << CAN_NODE_NPCR_RXSEL_Pos) & (uint32_t)CAN_NODE_NPCR_RXSEL_Msk);  
     
    pcan0->node->NPCR = pcan0->node->NPCR & ~(uint32_t)(CAN_NODE_NPCR_RXSEL_Msk << CAN_NODE_NPCR_RXSEL_Pos);
    pcan0->node->NPCR = pcan0->node->NPCR | (uint32_t)(CAN_NODE0_RXD_P1_0 << CAN_NODE_NPCR_RXSEL_Pos);    
    
    
    can0_disable_configuration_change();

    
    can0_enable_configuration_change();
    pcan0->set_speed(speed);
    can0_disable_configuration_change();
    
}
//-----------------------------------------
void can0_open (void)
{
  pcan0 = (_CAN *)&pcan0_buffer;
  pcan0->reg = (XMC_CAN_t *)CAN_BASE;
  pcan0->node = (XMC_CAN_NODE_t *)CAN_NODE0_BASE;
  pcan0->init = can0_init;
  pcan0->enable = can0_enable;
  pcan0->disable = can0_disable;
  pcan0->start = can0_reset_init_bit;
  pcan0->stop = can0_set_init_bit;
  pcan0->set_speed = can0_set_speed;
  pcan0->disable_mbox = can0_disable_mbox;
  pcan0->enable_mbox = can0_enable_mbox;
  pcan0->configure_mbox = can0_configure_mbox;
  pcan0->configure_ext_mbox = can0_configure_ext_mbox;
  pcan0->send = can0_send;
  pcan0->recv = can0_recv;

}

//-------------------------------------
//              CAN1
//-------------------------------------
void can1_enable (void)
{
  pscu->enable_peripheral_clock(SCU_CLK_CGATCLR0_MCAN0_Msk);
  //XMC_CAN_Init(CAN, XMC_CAN_CANCLKSRC_MCLK, 11000000); 
  // --- enable module controll --- //
  pcan1->reg->CLC = 2;
  pcan1->reg->MCR = (pcan1->reg->MCR & ~CAN_MCR_CLKSEL_Msk) | XMC_CAN_CANCLKSRC_MCLK ;
}
//-------------------------------------
void can1_disable (void)
{
    pscu->disable_peripheral_clock(SCU_CLK_CGATCLR0_MCAN0_Msk);
}

void can1_enable_configuration_change (void)
{
  pcan1->node->NCR |= (uint32_t)CAN_NODE_NCR_CCE_Msk;
}

void can1_disable_configuration_change (void)
{
  pcan1->node->NCR &= ~(uint32_t)CAN_NODE_NCR_CCE_Msk;
}

void can1_set_init_bit (void)
{
  pcan1->node->NCR |= (uint32_t)CAN_NODE_NCR_INIT_Msk;
}

void can1_reset_init_bit (void)
{
  pcan1->node->NCR &= ~(uint32_t)CAN_NODE_NCR_INIT_Msk;
}
//-------------------------------------
void can1_set_speed (long speed)
{
    uint32_t temp_brp = 12U ;
    uint32_t temp_tseg1 = 12U;
    uint32_t best_brp = 0U;
    uint32_t best_tseg1 = 1U;
    uint32_t best_tseg2 = 0U;
    uint32_t best_tbaud = 0U;
    uint32_t best_error = 10000U;
    
    //uint32_t can_frequency = 11000000;
    uint32_t can_frequency = 11000000;
    
    uint32_t baudrate = speed;
    uint32_t sample_point = 6000;
    uint32_t sjw = 3;    
    
  /*
   * Bit timing & sampling
   * Tq = (BRP+1)/Fcan if DIV8 = 0
   * Tq = 8*(BRP+1)/Fcan if DIV8 = 1
   * TSync = 1.Tq
   * TSeg1 = (TSEG1+1)*Tq                >= 3Tq
   * TSeg2 = (TSEG2+1)*Tq                >= 2Tq
   * Bit Time = TSync + TSeg1 + TSeg2    >= 8Tq
   *
   * Resynchronization:
   *
   * Tsjw = (SJW + 1)*Tq
   * TSeg1 >= Tsjw + Tprop
   * TSeg2 >= Tsjw
   */

    for (temp_brp = 1U; temp_brp <= 64U; temp_brp++)
    {
        uint32_t f_quanta = (uint32_t)((can_frequency * 10U) / temp_brp);
        uint32_t temp_tbaud = (uint32_t)(f_quanta / (baudrate));
        uint32_t temp_baudrate;
        uint32_t error;

        if((temp_tbaud % 10U) > 5U)
        {
            temp_tbaud = (uint32_t)(temp_tbaud / 10U);
            temp_tbaud++;
        }
        else
        {
            temp_tbaud = (uint32_t)(temp_tbaud / 10U);
        }

        if(temp_tbaud > 0U)
        {
            temp_baudrate = (uint32_t) (f_quanta / (temp_tbaud * 10U));
        }
        else
        {
            temp_baudrate = f_quanta / 10U;
            temp_tbaud = 1;
        }

        if(temp_baudrate >= baudrate)
        {
            error = temp_baudrate - baudrate;
        }
        else
        {
            error = baudrate - temp_baudrate;
        }

        if ((temp_tbaud <= 20U) && (best_error > error))
        {
            best_brp = temp_brp;
            best_tbaud = temp_tbaud;
            best_error = (error);

            if (error < 1000U)
                break;
        }
    }
    
    /* search for best sample point */
    best_error = 10000U;

    for (temp_tseg1 = 64U; temp_tseg1 >= 3U; temp_tseg1--)
    {
        uint32_t tempSamplePoint = ((temp_tseg1 + 1U) * 10000U) / best_tbaud;
        uint32_t error;
        
        if (tempSamplePoint >= sample_point)
            error = tempSamplePoint  - sample_point;
        else
            error = sample_point  - tempSamplePoint;

        if (best_error > error)
        {
            best_tseg1 = temp_tseg1;
            best_error = error;
        }
        
        if (tempSamplePoint < (sample_point))
            break;
    }    
   
    best_tseg2 = best_tbaud - best_tseg1 - 1U;

    can1_enable_configuration_change();
    /* Configure bit timing register */
    pcan1->node->NBTR = (((uint32_t)(best_tseg2 - 1u) << CAN_NODE_NBTR_TSEG2_Pos) & (uint32_t)CAN_NODE_NBTR_TSEG2_Msk) |
                   ((((uint32_t)((uint32_t)(sjw)-1U) << CAN_NODE_NBTR_SJW_Pos)) & (uint32_t)CAN_NODE_NBTR_SJW_Msk)|
                   (((uint32_t)(best_tseg1-1U) << CAN_NODE_NBTR_TSEG1_Pos) & (uint32_t)CAN_NODE_NBTR_TSEG1_Msk)|
                   (((uint32_t)(best_brp - 1U) << CAN_NODE_NBTR_BRP_Pos) & (uint32_t)CAN_NODE_NBTR_BRP_Msk)|
                   (((uint32_t)0U << CAN_NODE_NBTR_DIV8_Pos) & (uint32_t)CAN_NODE_NBTR_DIV8_Msk);
    can1_disable_configuration_change();    
}
//-------------------------------------
void can1_setup_clock (void)
{
  //int tq;       // number of sample points per bit
  //int prop;     // signal delivery and return time in tq (bus driver delay + receiver delay = 30ns + bus signal travel 1m ~ 5ns)
  //int BRP;
  //int PHASE1,PHASE2;
  //int SJW;
  
  uint32_t peripheral_frequency;
  uint32_t can_frequency = 11000000;
  uint32_t freq_n, freq_f;
  //uint32_t step;
  int normal_divider;
  //int can_divider_mode;
  uint32_t step_n, step_f;
  uint32_t can_frequency_khz;
  uint32_t peripheral_frequency_khz;
  
  
  peripheral_frequency = ((pcan1->reg->MCR & CAN_MCR_CLKSEL_Msk) >> CAN_MCR_CLKSEL_Pos);
  
  /* Normal divider mode */
  if ((peripheral_frequency / can_frequency) > 1024)
      step_n = 0;
  else
      step_n = 1024U - (peripheral_frequency / can_frequency);
  
  //step_n = (uint32_t)min(0, 1023U);
  freq_n = (uint32_t) (peripheral_frequency / (1024U - step_n));
  
  /* Fractional divider mode */
  can_frequency_khz = (uint32_t) (can_frequency >> 6);
  peripheral_frequency_khz = (uint32_t)(peripheral_frequency >> 6);

  if (((1024U * can_frequency_khz) / peripheral_frequency_khz) < 1023)
      step_f = (1024U * can_frequency_khz) / peripheral_frequency_khz;
  else
      step_f = 1023; 
  
  //step_f = (uint32_t)(min( (((1024U * can_frequency_khz) / peripheral_frequency_khz) ), 1023U ));
  freq_f = (uint32_t)((peripheral_frequency_khz * step_f) / 1024U);
  freq_f = freq_f << 6;

  if ((uint32_t)(can_frequency - freq_n) <= (can_frequency - freq_f))
      normal_divider = 1;
  else
      normal_divider = 0;
  
  //step   = (normal_divider != 0U) ? step_n : step_f;
  //can_divider_mode = (normal_divider != 0U) ? XMC_CAN_DM_NORMAL : XMC_CAN_DM_FRACTIONAL;

  pcan1->reg->FDR &= (uint32_t) ~(CAN_FDR_DM_Msk | CAN_FDR_STEP_Msk);
  //pcan1->reg->FDR |= ((uint32_t)can_divider_mode << CAN_FDR_DM_Pos) | ((uint32_t)step << CAN_FDR_STEP_Pos);  
   pcan1->reg->FDR = 0x80EA;
}
//-------------------------------------
void can1_disable_mbox (int mbox)
{
    //pcan1->reg->CAN_MB[mbox].CAN_MMR &= 0xf7ffffff;
}
//-------------------------------------
void can1_enable_mbox (int mbox, int mode)
{
    //pcan1->reg->CAN_MB[mbox].CAN_MMR |= (mode << 24);
}
//-------------------------------------
void can1_configure_mbox (int mbox, uint32_t id, uint32_t mask, int prior, int mode)
{
    //uint32_t reg;
    CAN_MO_TypeDef *can_mbox;
    
    if (mbox > 31)
        return;
    
    can_mbox = (CAN_MO_TypeDef *)&CAN_MO->MO[mbox];    
    can_mbox->MOIPR = (((uint32_t)(mbox / 32) << (CAN_MO_MOIPR_MPN_Pos + 5U)) | ((uint32_t)(mbox % 32) << CAN_MO_MOIPR_MPN_Pos));

    /* Disable Message object */
    can_mbox->MOCTR = CAN_MO_MOCTR_RESMSGVAL_Msk;

    id &= 0x7FF;
    //id &= (uint32_t) ~(CAN_MO_MOAR_ID_Msk);
    can_mbox->MOAR = (1 << 30) | (id << XMC_CAN_MO_MOAR_STDID_Pos);  //0x83fc0000;
    can_mbox->MOAMR = (1 << 29) | (mask << XMC_CAN_MO_MOAR_STDID_Pos); //0x23fc0000
     
    /* Check whether message object is transmit message object */
    if (mode == XMC_CAN_MO_TYPE_TRANSMSGOBJ)
    {
        /* Set MO as Transmit message object  */
        //XMC_CAN_MO_UpdateData(can_mo);
        can_mbox->MOCTR = CAN_MO_MOCTR_SETDIR_Msk;
    }
    else
    {
        /* Set MO as Receive message object and set RXEN bit */
        can_mbox->MOCTR = CAN_MO_MOCTR_RESDIR_Msk;
    }

    /* Reset RTSEL and Set MSGVAL ,TXEN0 and TXEN1 bits */
    can_mbox->MOCTR = (CAN_MO_MOCTR_SETTXEN0_Msk | CAN_MO_MOCTR_SETTXEN1_Msk | CAN_MO_MOCTR_SETMSGVAL_Msk |
                                 CAN_MO_MOCTR_SETRXEN_Msk | CAN_MO_MOCTR_RESRTSEL_Msk);
    
    //pcan1->reg->PANCTR = (2 << 24) | (mbox << 16) | 2;
    
    /* allocation of MO to node list */
    pcan1->reg->PANCTR = (((uint32_t)XMC_CAN_PANCMD_STATIC_ALLOCATE << CAN_PANCTR_PANCMD_Pos) & (uint32_t)CAN_PANCTR_PANCMD_Msk) |
		        (((uint32_t)mbox << CAN_PANCTR_PANAR1_Pos) & (uint32_t)CAN_PANCTR_PANAR1_Msk) |
		        (((uint32_t)(1 + 1) << CAN_PANCTR_PANAR2_Pos) & (uint32_t)CAN_PANCTR_PANAR2_Msk);
  
    
    //XMC_CAN_PanelControl(obj, XMC_CAN_PANCMD_STATIC_ALLOCATE,mbox,(1 + 1U));
    /* wait until panel as done the command */
    while (pcan1->reg->PANCTR & CAN_PANCTR_BUSY_Msk);    
}
//-------------------------------------
void can1_configure_ext_mbox (int mbox, uint32_t id, int prior, int mode)
{
  /*
    pcan1->reg->CAN_MB[mbox].CAN_MMR = mode + (prior << 16);
    pcan1->reg->CAN_MB[mbox].CAN_MID = (id & 0x1fffffff) | (1 << 29);

    if ((mode == CAN_MMR_MOT_MB_RX) || (mode == CAN_MMR_MOT_MB_RX_OVERWRITE))
    {
        pcan1->reg->CAN_MB[mbox].CAN_MAM = 0x1fffffff;
        pcan1->reg->CAN_MB[mbox].CAN_MCR |= CAN_MCR_MTCR;   // allow receive
    }
*/
}
//-------------------------------------
int can1_mbox_rx_complete (int mbox)
{
    CAN_MO_TypeDef *can_mbox;
    
    if (mbox > 31)
        return 0;  
    
    can_mbox = (CAN_MO_TypeDef *)&CAN_MO->MO[mbox];
    
    if (can_mbox->MOSTAT & CAN_MO_MOSTAT_NEWDAT_Msk)
        return 1;
    
    return 0;
}
//-------------------------------------
int can1_recv (int mbox, char *buff, uint32_t *_id, int to)
{
    CAN_MO_TypeDef *can_mbox;
    uint32_t *plong = (uint32_t *)buff;
    int timeout = kern_get_tickcount() + to;
    int dlc;
    //uint32_t id;
    
    can_mbox = (CAN_MO_TypeDef *)&CAN_MO->MO[mbox];
    
    while (can1_mbox_rx_complete(mbox) == 0)
    {
        if (timeout <= kern_get_tickcount())
        {
            return 0;
        }
    }
    
    // --- clear NEWDAT
    plong[0] = can_mbox->MODATAL;
    plong[1] = can_mbox->MODATAH;
    
    // --- clear NEWDAT and RXUPD
    
    can_mbox->MOCTR = (CAN_MO_MOSTAT_NEWDAT_Msk | CAN_MO_MOSTAT_RXUPD_Msk);
    can_mbox->MOCTR = CAN_MO_MOCTR_SETRXEN_Msk;
    
    // --- for EXTENDED --- //
    //*_id = (can_mbox->MOAR >> 0) & 0x1FFFFFFF;  // --- received ID
    
    *_id = (can_mbox->MOAR >> 18) & 0x1FFFFFFF;  // --- received ID
    dlc = (can_mbox->MOFCR >> CAN_MO_MOFCR_DLC_Pos);
    dlc &= 0x0F;

    return dlc;
}
//-------------------------------------
int can1_send (int mbox, char *buff, int dlc, int to)
{
    uint32_t *plong = (uint32_t *)buff;
    int timeout = kern_get_tickcount() + to;
    CAN_MO_TypeDef *can_mbox;
    
    if (mbox > 31)
        return 0;
    
    can_mbox = (CAN_MO_TypeDef *)&CAN_MO->MO[mbox];  

    can_mbox->MOCTR = CAN_MO_MOCTR_RESMSGVAL_Msk;
    /* Configure data length */
    can_mbox->MOFCR = ((can_mbox->MOFCR) & ~(uint32_t)(CAN_MO_MOFCR_DLC_Msk)) |
                                (((uint32_t) dlc << CAN_MO_MOFCR_DLC_Pos) & (uint32_t)CAN_MO_MOFCR_DLC_Msk);
    /* Configure Data registers*/
    can_mbox->MODATAL = plong[0];
    can_mbox->MODATAH = plong[1];
    /* Reset RTSEL and Set MSGVAL ,TXEN0 and TXEN1 bits */
    can_mbox->MOCTR = (CAN_MO_MOCTR_SETNEWDAT_Msk| CAN_MO_MOCTR_SETMSGVAL_Msk |CAN_MO_MOCTR_RESRTSEL_Msk);    
    
    // --- transmit message --- //
    uint32_t mo_type = (uint32_t)(((can_mbox->MOSTAT) & CAN_MO_MOSTAT_MSGVAL_Msk) >> CAN_MO_MOSTAT_MSGVAL_Pos);
    uint32_t mo_transmission_ongoing = (uint32_t) ((can_mbox->MOSTAT) & CAN_MO_MOSTAT_TXRQ_Msk) >> CAN_MO_MOSTAT_TXRQ_Pos;
    
    /* check if message is disabled */
    if (mo_type == 0U)
        return 0;
    else if (mo_transmission_ongoing == 1U)
        return 0;
    else
    {
        /* set TXRQ bit */
        can_mbox-> MOCTR = CAN_MO_MOCTR_SETTXRQ_Msk | CAN_MO_MOCTR_SETTXEN0_Msk | CAN_MO_MOCTR_SETTXEN1_Msk;
    }
    
    return 1;
}
//-------------------------------------
void can1_init (long speed)
{
    int i;

    pcan1->enable();
    can1_setup_clock();
  
    pgpio_2->set_input(CAN1_RX);
    pgpio_2->set_input(CAN1_TX);    
  
    /* Set mode */
    pgpio_2->reg->IOCR[CAN1_TX >> 2U] &= 0x00FFFFFF;
    pgpio_2->reg->IOCR[CAN1_TX >> 2U] |= (uint32_t)XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT9 << (PORT_IOCR_PC_Size * (CAN1_TX & 0x3U));  
  
    // --- configure CAN1 pins --- //
    pgpio_2->set_input(CAN1_RX);
    //pgpio_2->set_input(CAN1_TX);     
    
    //pgpio_2->reg->IOCR[CAN1_RX >> 2U] &= 0x0000FFFF;
    //pgpio_2->reg->IOCR[CAN1_RX >> 2U] |= (uint32_t)XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT9 << (PORT_IOCR_PC_Size * (CAN1_RX & 0x3U));        
    
    can1_enable_configuration_change();
    
    // --- disable Loop Back mode --- //
    pcan1->node->NPCR = (pcan1->node->NPCR) & ~((uint32_t)CAN_NODE_NPCR_LBM_Msk << CAN_NODE_NPCR_LBM_Pos);
    
    // --- before 2020.01.20 --- //
    //pcan1->node->NPCR = ((pcan1->node->NPCR) & ~(uint32_t)(CAN_NODE_NPCR_RXSEL_Msk)) | 
    //    (((uint32_t)CAN_NODE1_RXD_P2_10 << CAN_NODE_NPCR_RXSEL_Pos) & (uint32_t)CAN_NODE_NPCR_RXSEL_Msk);  
    
    pcan1->node->NPCR &= ~(uint32_t)(CAN_NODE_NPCR_RXSEL_Msk << CAN_NODE_NPCR_RXSEL_Pos);
    pcan1->node->NPCR |= (uint32_t)(CAN_NODE1_RXD_P2_10 << CAN_NODE_NPCR_RXSEL_Pos);
      
    can1_disable_configuration_change();

    
    can1_enable_configuration_change();
    pcan1->set_speed(speed);
    can1_disable_configuration_change();

    
}
//-----------------------------------------
void can1_open (void)
{
  pcan1 = (_CAN *)&pcan1_buffer;
  pcan1->reg = (XMC_CAN_t *)CAN_BASE;
  pcan1->node = (XMC_CAN_NODE_t *)CAN_NODE1_BASE;
  pcan1->init = can1_init;
  pcan1->enable = can1_enable;
  pcan1->disable = can1_disable;
  pcan1->start = can1_reset_init_bit;
  pcan1->stop = can1_set_init_bit;
  pcan1->set_speed = can1_set_speed;
  pcan1->disable_mbox = can1_disable_mbox;
  pcan1->enable_mbox = can1_enable_mbox;
  pcan1->configure_mbox = can1_configure_mbox;
  pcan1->configure_ext_mbox = can1_configure_ext_mbox;
  pcan1->send = can1_send;
  pcan1->recv = can1_recv;

}
