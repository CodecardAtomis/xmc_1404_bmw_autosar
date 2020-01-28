#include "XMC1400.h"
#include "gpio.h"
#include "scu.h"
#include "uart.h"

#include "kern\kern_types.h"
#include "kern\kern.h"

#define P1_2    2
#define P1_3    3

#define US0_TX  P1_2
#define US0_RX  P1_3

extern _GPIO_1    *pgpio_1;
extern _SCU       *pscu;

//----------------------------------------
_US0  *pus0;
_US0   pus0_buffer;
//----------------------------------------

//------------- USART 0 -----------------//
//
//---------------------------------------//
void us0_set_baud_rate (long val)       // baud = MCK/(16 * val)
{
  uint32_t peripheral_clock;
  
  uint32_t oversampling = XMC_UART_CH_OVERSAMPLING;
  uint32_t clock_divider;
  uint32_t clock_divider_min;

  uint32_t pdiv;
  uint32_t pdiv_int;
  uint32_t pdiv_int_min;
  
  uint32_t pdiv_frac;
  uint32_t pdiv_frac_min;

  peripheral_clock = 48000000 / 100U;
  //peripheral_clock = 20000000 / 100U;
  val = val / 100U;

  clock_divider_min = 1U;
  pdiv_int_min = 1U;
  pdiv_frac_min = 0x3ffU;

  for (clock_divider = 1023U; clock_divider > 0U; --clock_divider)
  {
      pdiv = ((peripheral_clock * clock_divider) / (val * oversampling));
      pdiv_int = pdiv >> 10U;
      pdiv_frac = pdiv & 0x3ffU;

      if ((pdiv_int < 1024U) && (pdiv_frac < pdiv_frac_min))
      {
        pdiv_frac_min = pdiv_frac;
        pdiv_int_min = pdiv_int;
        clock_divider_min = clock_divider;
      }
  }

    /* fFD = fPB */
    /* FDR.DM = 02b (Fractional divider mode) */
    pus0->reg->FDR &= ~(USIC_CH_FDR_DM_Msk | USIC_CH_FDR_STEP_Msk);
    
    pus0->reg->FDR |= (0x02UL << USIC_CH_FDR_DM_Pos) | (clock_divider_min << USIC_CH_FDR_STEP_Pos);  
  
    pus0->reg->BRG &= ~(USIC_CH_BRG_PCTQ_Msk | USIC_CH_BRG_DCTQ_Msk | USIC_CH_BRG_PDIV_Msk | USIC_CH_BRG_CLKSEL_Msk | USIC_CH_BRG_PPPEN_Msk);
  
    //pus0->reg->BRG = 0x43C00; 
    pus0->reg->BRG = (pus0->reg->BRG & ~(USIC_CH_BRG_DCTQ_Msk |
                                     USIC_CH_BRG_PDIV_Msk |
                                     USIC_CH_BRG_PCTQ_Msk |
                                     USIC_CH_BRG_PPPEN_Msk)) |
                   ((oversampling - 1U) << USIC_CH_BRG_DCTQ_Pos) |
                   ((pdiv_int_min - 1U) << USIC_CH_BRG_PDIV_Pos);  

}

int us0_get_transmit_status ()
{
    return pus0->reg->PSR_ASCMode;
}

void us0_send_byte (char b)
{
  /* Check FIFO size */
  if ((pus0->reg->TBCTR & USIC_CH_TBCTR_SIZE_Msk) == 0UL)
  {
    /* Wait till the Transmit Buffer is free for transmission */
    while(us0_get_transmit_status() == XMC_USIC_CH_TBUF_STATUS_BUSY);
  
    /* Clear the Transmit Buffer indication flag */
    pus0->reg->PSCR = XMC_UART_CH_STATUS_FLAG_TRANSMIT_BUFFER_INDICATION;
  
    /*Transmit data */
    pus0->reg->TBUF[0U] = b;
    //pus0->reg->IN[0U] = b;
    //USIC0_CH1->IN[0] = b;
  }
  else
  {
    pus0->reg->IN[0U] = b;
    //USIC0_CH1->IN[0] = b;
  }
}

char us0_recv_byte (long time)
{
  char retval = 0;
  long timeout = kern_get_tickcount() + time;
  
  pus0->last_com_err = 0;
  long regStatus = pus0->reg->RBUFSR;
  
  if ((regStatus & (USIC_CH_RBUFSR_RDV1_Msk | USIC_CH_RBUFSR_RDV0_Msk)) != 0)
  {
      /* Check FIFO size */
      if ((pus0->reg->RBCTR & USIC_CH_RBCTR_SIZE_Msk) == 0U)
      {
          retval = (char)pus0->reg->RBUF;
      }
      else
      {
          retval = (char)pus0->reg->OUTR;
      }
  }
  else
      pus0->last_com_err = 1;
  
  return retval;
}

void us0_enable (void)
{
  pscu->enable_peripheral_clock(SCU_CLK_CGATSTAT0_USIC0_Msk);
  
  /* USIC channel switched on*/
  pus0->reg->KSCFG |= (USIC_CH_KSCFG_MODEN_Msk | USIC_CH_KSCFG_BPMODEN_Msk);
  while ((pus0->reg->KSCFG & USIC_CH_KSCFG_MODEN_Msk) == 0U);  
}

void us0_reset (void)
{
  //pua1->reg->UART_CR = 0x0C;
}

void us0_rx_enable (void)
{
  pgpio_1->set_input(US0_RX);
  
  pus0->reg->DXCR[XMC_UART_CH_INPUT_RXD] &= ~(USIC_CH_DX0CR_DSEL_Msk);
}


void us0_tx_enable (void)
{  
    /* Initialize UART_TX (DOUT)*/
    /* P1.2 as output controlled by ALT7 = U0C1.DOUT0 */
    //Pin_set_mode(1, 2, OUTPUT_PP_AF7);  
  
    /* Set mode */
    pgpio_1->reg->IOCR[US0_TX >> 2U] &= ~(0xFC << 16);
    pgpio_1->reg->IOCR[US0_TX >> 2U] |= (uint32_t)XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT7 << (PORT_IOCR_PC_Size * (US0_TX & 0x3U));   
}

void us0_set_mode (void)
{                                   
    //int stop_bits = 1;
    //int oversampling = 0;
    unsigned long data_bits = 8 - 1;
    unsigned long frame_length = 8 - 1;
    int parity_mode = XMC_USIC_CH_PARITY_MODE_NONE;
  
  
    /* Configuration of USIC Shift Control */
    /* SCTR.FLE = 8 (Frame Length) */
    /* SCTR.WLE = 8 (Word Length) */
    /* SCTR.TRM = 1 (Transmission Mode) */
    /* SCTR.PDL = 1 (This bit defines the output level at the shift data output signal when no data is available for transmission) */
    pus0->reg->SCTR &= ~(USIC_CH_SCTR_TRM_Msk | USIC_CH_SCTR_FLE_Msk | USIC_CH_SCTR_WLE_Msk);
    pus0->reg->SCTR |= USIC_CH_SCTR_PDL_Msk | (0x01UL << USIC_CH_SCTR_TRM_Pos) | (frame_length << USIC_CH_SCTR_FLE_Pos) | (data_bits << USIC_CH_SCTR_WLE_Pos);   
  
    /* Configuration of USIC Transmit Control/Status Register */
    /* TBUF.TDEN = 1 (TBUF Data Enable: A transmission of the data word in TBUF can be started if TDV = 1 */
    /* TBUF.TDSSM = 1 (Data Single Shot Mode: allow word-by-word data transmission which avoid sending the same data several times*/
    pus0->reg->TCSR &= ~(USIC_CH_TCSR_TDEN_Msk);
    pus0->reg->TCSR |= USIC_CH_TCSR_TDSSM_Msk | (0x01UL << USIC_CH_TCSR_TDEN_Pos);  
  
  
    /* Configuration of Protocol Control Register */
    /* PCR.SMD = 1 (Sample Mode based on majority) */
    /* PCR.STPB = 0 (1x Stop bit) */
    /* PCR.SP = 5 (Sample Point) */
    /* PCR.PL = 0 (Pulse Length is equal to the bit length) */
    pus0->reg->PCR &= ~(USIC_CH_PCR_ASCMode_STPB_Msk | USIC_CH_PCR_ASCMode_SP_Msk | USIC_CH_PCR_ASCMode_PL_Msk);
    pus0->reg->PCR |= USIC_CH_PCR_ASCMode_SMD_Msk | (9 << USIC_CH_PCR_ASCMode_SP_Pos);
    
    /* Configure Transmit Buffer */
    /* Standard transmit buffer event is enabled */
    /* Define start entry of Transmit Data FIFO buffer DPTR = 0*/
    pus0->reg->TBCTR &= ~(USIC_CH_TBCTR_SIZE_Msk | USIC_CH_TBCTR_DPTR_Msk);

    /* Set Transmit Data Buffer to 64 */
    pus0->reg->TBCTR |= 0x06UL << USIC_CH_TBCTR_SIZE_Pos;    
}


void us0_init (void)
{
    //pus0->reset();
    pus0->enable();
    //pus0->set_baud_rate(9600);             // 10 - 38400bps, 650 - 9600 bps, 54 - 115200bps
    pus0->set_baud_rate(115200);
    pus0->set_mode();
    //pus0->set_parity();
    pus0->rx_enable();
    pus0->tx_enable();
  
    /* Configuration of Channel Control Register */
    /* CCR.PM = 00 ( Disable parity generation) */
    /* CCR.MODE = 2 (ASC mode enabled. Note: 0 (USIC channel is disabled)) */
    pus0->reg->CCR &= ~(USIC_CH_CCR_PM_Msk | USIC_CH_CCR_MODE_Msk);
    pus0->reg->CCR |= 0x02UL << USIC_CH_CCR_MODE_Pos;  
}

#define FDR_STEP 590UL
#define BRG_PDIV 4UL
#define BRG_DCTQ 15UL
#define BRG_PCTQ 0UL

#define INPUT           0x00U
#define INPUT_PD        0x08U
#define INPUT_PU        0x10U
#define INPUT_PPS       0x18U
#define INPUT_INV       0x20U
#define INPUT_INV_PD    0x28U
#define INPUT_INV_PU    0x30U
#define INPUT_INV_PPS   0x38U
#define OUTPUT_PP_GP    0x80U
#define OUTPUT_PP_AF1   0x84U
#define OUTPUT_PP_AF2   0x88U
#define OUTPUT_PP_AF3   0x8CU
#define OUTPUT_PP_AF4   0x90U
#define OUTPUT_PP_AF5   0x94U
#define OUTPUT_PP_AF6   0x98U
#define OUTPUT_PP_AF7   0x9CU
#define OUTPUT_PP_AF8   0xA0U
#define OUTPUT_PP_AF9   0xA4U
#define OUTPUT_OD_GP    0xC0U
#define OUTPUT_OD_AF1   0xC4U
#define OUTPUT_OD_AF2   0xC8U
#define OUTPUT_OD_AF3   0xCCU
#define OUTPUT_OD_AF4   0XD0U
#define OUTPUT_OD_AF5   0xD4U
#define OUTPUT_OD_AF6   0xD8U
#define OUTPUT_OD_AF7   0XDCU
#define OUTPUT_OD_AF8   0XE0U
#define OUTPUT_OD_AF9   0XE4U

#define IOCR_PIN_0    IOCR0
#define IOCR_PIN_1    IOCR0
#define IOCR_PIN_2    IOCR0
#define IOCR_PIN_3    IOCR0
#define IOCR_PIN_4    IOCR4
#define IOCR_PIN_5    IOCR4
#define IOCR_PIN_6    IOCR4
#define IOCR_PIN_7    IOCR4
#define IOCR_PIN_8    IOCR8
#define IOCR_PIN_9    IOCR8
#define IOCR_PIN_10   IOCR8
#define IOCR_PIN_11   IOCR8
#define IOCR_PIN_12  IOCR12
#define IOCR_PIN_13  IOCR12
#define IOCR_PIN_14  IOCR12
#define IOCR_PIN_15  IOCR12

#define IOCR_PIN_BITPOS_0    0UL
#define IOCR_PIN_BITPOS_1    8UL
#define IOCR_PIN_BITPOS_2   16UL
#define IOCR_PIN_BITPOS_3   24UL
#define IOCR_PIN_BITPOS_4    0UL
#define IOCR_PIN_BITPOS_5    8UL
#define IOCR_PIN_BITPOS_6   16UL
#define IOCR_PIN_BITPOS_7   24UL
#define IOCR_PIN_BITPOS_8    0UL
#define IOCR_PIN_BITPOS_9    8UL
#define IOCR_PIN_BITPOS_10  16UL
#define IOCR_PIN_BITPOS_11  24UL
#define IOCR_PIN_BITPOS_12   0UL
#define IOCR_PIN_BITPOS_13   8UL
#define IOCR_PIN_BITPOS_14  16UL
#define IOCR_PIN_BITPOS_15  24UL

#define IOCR_NUM(pin)     IOCR_PIN_##pin
#define IOCR_BIT_POS(pin) IOCR_PIN_BITPOS_##pin

#define PHCR_NUM(pin)     PHCR_PIN_##pin
#define PHCR_BIT_POS(pin) PHCR_PIN_BITPOS_##pin

#define Pin_set_mode(port, pin, mode)    PORT##port->IOCR_NUM(pin) &= ~(0x000000FCUL << IOCR_BIT_POS(pin)); \
                                         PORT##port->IOCR_NUM(pin) |= mode << IOCR_BIT_POS(pin)

void _UART_Init()
{
    // Disable clock gating to USIC0
    SCU_GENERAL->PASSWD = 0x000000C0UL; // disable bit protection
    SCU_CLK->CGATCLR0 |= SCU_CLK_CGATCLR0_USIC0_Msk;
    SCU_GENERAL->PASSWD = 0x000000C3UL; // enable bit protection

    /* Enable the module kernel clock and the module functionality  */
    USIC0_CH1->KSCFG |= USIC_CH_KSCFG_MODEN_Msk | USIC_CH_KSCFG_BPMODEN_Msk;

    /* fFD = fPB */
    /* FDR.DM = 02b (Fractional divider mode) */
    USIC0_CH1->FDR &= ~(USIC_CH_FDR_DM_Msk | USIC_CH_FDR_STEP_Msk);
    USIC0_CH1->FDR |= (0x02UL << USIC_CH_FDR_DM_Pos) | (FDR_STEP << USIC_CH_FDR_STEP_Pos);

    /* Configure baud rate generator */
    /* BAUDRATE = fCTQIN/(BRG.PCTQ x BRG.DCTQ) */
    /* CLKSEL = 0 (fPIN = fFD), CTQSEL = 00b (fCTQIN = fPDIV), PPPEN = 0 (fPPP=fPIN) */
    USIC0_CH0->BRG &= ~(USIC_CH_BRG_PCTQ_Msk | USIC_CH_BRG_DCTQ_Msk | USIC_CH_BRG_PDIV_Msk | USIC_CH_BRG_CLKSEL_Msk | USIC_CH_BRG_PPPEN_Msk);
    USIC0_CH1->BRG |= (BRG_PCTQ << USIC_CH_BRG_PCTQ_Pos) | (BRG_DCTQ << USIC_CH_BRG_DCTQ_Pos) | (BRG_PDIV << USIC_CH_BRG_PDIV_Pos);

    /* Configuration of USIC Shift Control */
    /* SCTR.FLE = 8 (Frame Length) */
    /* SCTR.WLE = 8 (Word Length) */
    /* SCTR.TRM = 1 (Transmission Mode) */
    /* SCTR.PDL = 1 (This bit defines the output level at the shift data output signal when no data is available for transmission) */
    USIC0_CH1->SCTR &= ~(USIC_CH_SCTR_TRM_Msk | USIC_CH_SCTR_FLE_Msk | USIC_CH_SCTR_WLE_Msk);
    USIC0_CH1->SCTR |= USIC_CH_SCTR_PDL_Msk | (0x01UL << USIC_CH_SCTR_TRM_Pos) | (0x07UL << USIC_CH_SCTR_FLE_Pos) | (0x07UL << USIC_CH_SCTR_WLE_Pos);

    /* Configuration of USIC Transmit Control/Status Register */
    /* TBUF.TDEN = 1 (TBUF Data Enable: A transmission of the data word in TBUF can be started if TDV = 1 */
    /* TBUF.TDSSM = 1 (Data Single Shot Mode: allow word-by-word data transmission which avoid sending the same data several times*/
    USIC0_CH1->TCSR &= ~(USIC_CH_TCSR_TDEN_Msk);
    USIC0_CH1->TCSR |= USIC_CH_TCSR_TDSSM_Msk | (0x01UL << USIC_CH_TCSR_TDEN_Pos);

    /* Configuration of Protocol Control Register */
    /* PCR.SMD = 1 (Sample Mode based on majority) */
    /* PCR.STPB = 0 (1x Stop bit) */
    /* PCR.SP = 5 (Sample Point) */
    /* PCR.PL = 0 (Pulse Length is equal to the bit length) */
    USIC0_CH1->PCR &= ~(USIC_CH_PCR_ASCMode_STPB_Msk | USIC_CH_PCR_ASCMode_SP_Msk | USIC_CH_PCR_ASCMode_PL_Msk);
    USIC0_CH1->PCR |= USIC_CH_PCR_ASCMode_SMD_Msk | (9 << USIC_CH_PCR_ASCMode_SP_Pos);

    /* Configure Transmit Buffer */
    /* Standard transmit buffer event is enabled */
    /* Define start entry of Transmit Data FIFO buffer DPTR = 0*/
    USIC0_CH1->TBCTR &= ~(USIC_CH_TBCTR_SIZE_Msk | USIC_CH_TBCTR_DPTR_Msk);

    /* Set Transmit Data Buffer to 64 */
    USIC0_CH1->TBCTR |= 0x06UL << USIC_CH_TBCTR_SIZE_Pos;

    /* Initialize UART_RX (DX0)*/
    /* P1.3 as input */
    Pin_set_mode(1, 3, INPUT);
    //pgpio_1->set_input(US0_RX);

    /* Select P1.3 as input for USIC DX0 */
    USIC0_CH1->DX0CR &= ~(USIC_CH_DX0CR_DSEL_Msk);

    /* Initialize UART_TX (DOUT)*/
    /* P1.2 as output controlled by ALT7 = U0C1.DOUT0 */
    Pin_set_mode(1, 2, OUTPUT_PP_AF7);

    /* Configuration of Channel Control Register */
    /* CCR.PM = 00 ( Disable parity generation) */
    /* CCR.MODE = 2 (ASC mode enabled. Note: 0 (USIC channel is disabled)) */
    USIC0_CH1->CCR &= ~(USIC_CH_CCR_PM_Msk | USIC_CH_CCR_MODE_Msk);
    USIC0_CH1->CCR |= 0x02UL << USIC_CH_CCR_MODE_Pos;
}

//-------------------------------------------
void us0_open (void)
{
  pus0 = (_US0 *)&pus0_buffer;
  //pus0->reg = (XMC_USIC_CH_t *)USIC0_BASE;
  pus0->reg = (XMC_USIC_CH_t *)USIC0_CH1_BASE;

  pus0->init = us0_init;
  pus0->enable = us0_enable;
  pus0->enable = us0_enable;
  pus0->rx_enable = us0_rx_enable;
  pus0->tx_enable = us0_tx_enable; 
  pus0->set_mode = us0_set_mode;
  pus0->send_byte = us0_send_byte;
  pus0->recv_byte = us0_recv_byte;
  pus0->set_baud_rate = us0_set_baud_rate;  
}
