#include "XMC1400.h"

#define  XMC_USIC_CH_PARITY_MODE_NONE      0x0UL  /**< Disable parity mode */
#define  XMC_USIC_CH_PARITY_MODE_EVEN      0x2UL << USIC_CH_CCR_PM_Pos  /**< Enable even parity mode */
#define  XMC_USIC_CH_PARITY_MODE_ODD       0x3UL << USIC_CH_CCR_PM_Pos   /**< Enable odd parity mode */

#define XMC_USIC_CH_OPERATING_MODE_UART    (0x2UL << USIC_CH_CCR_MODE_Pos) /**< UART mode */

#define XMC_UART_CH_OVERSAMPLING (16UL)
#define XMC_UART_CH_OVERSAMPLING_MIN_VAL (4UL)

#define USIC_CH_DXCR_DSEL_Msk  USIC_CH_DX0CR_DSEL_Msk
#define USIC_CH_DXCR_DSEL_Pos  USIC_CH_DX0CR_DSEL_Pos

#define  USIC_CH_FDR_DM_Pos                             (14UL) 
#define  XMC_USIC_CH_BRG_CLOCK_DIVIDER_MODE_DISABLED    0x0UL /**< Baudrate generator clock divider: Disabled */
#define  XMC_USIC_CH_BRG_CLOCK_DIVIDER_MODE_NORMAL      (0x1UL << USIC_CH_FDR_DM_Pos) /**< Baudrate generator clock divider: Normal mode */
#define  XMC_USIC_CH_BRG_CLOCK_DIVIDER_MODE_FRACTIONAL  (0x2UL << USIC_CH_FDR_DM_Pos)  /**< Baudrate generator clock divider: Fractional mode */

#define  XMC_USIC_CH_TBUF_STATUS_IDLE                           0x0UL                 /**< Transfer buffer is currently idle*/
#define  XMC_USIC_CH_TBUF_STATUS_BUSY                           USIC_CH_TCSR_TDV_Msk   /**< Transfer buffer is currently busy*/
#define  XMC_UART_CH_STATUS_FLAG_TRANSMIT_BUFFER_INDICATION     USIC_CH_PSR_ASCMode_TBIF_Msk 

/**
 * USIC channel inputs
 */
typedef enum XMC_USIC_CH_INPUT
{
  XMC_USIC_CH_INPUT_DX0, /**< DX0 input */
  XMC_USIC_CH_INPUT_DX1, /**< DX1 input */
  XMC_USIC_CH_INPUT_DX2, /**< DX2 input */
  XMC_USIC_CH_INPUT_DX3, /**< DX3 input */
  XMC_USIC_CH_INPUT_DX4, /**< DX4 input */
  XMC_USIC_CH_INPUT_DX5  /**< DX5 input */
} XMC_USIC_CH_INPUT_t;


#define  XMC_UART_CH_INPUT_RXD          0UL   /**< UART input stage DX0*/
#define  XMC_UART_CH_INPUT_RXD1         3UL /**< UART input stage DX3*/
#define  XMC_UART_CH_INPUT_RXD2         5UL  /**< UART input stage DX5*/
    
typedef struct XMC_USIC_CH
{
  __I  uint32_t  RESERVED0;
  __I  uint32_t  CCFG;			/**< Channel configuration register*/
  __I  uint32_t  RESERVED1;
  __IO uint32_t  KSCFG;			/**< Kernel state configuration register*/
  __IO uint32_t  FDR;			/**< Fractional divider configuration register*/
  __IO uint32_t  BRG;			/**< Baud rate generator register*/
  __IO uint32_t  INPR;			/**< Interrupt node pointer register*/
  __IO uint32_t  DXCR[6];		/**< Input control registers DX0 to DX5.*/
  __IO uint32_t  SCTR;			/**< Shift control register*/
  __IO uint32_t  TCSR;

  union {
    __IO uint32_t  PCR_IICMode;	/**< I2C protocol configuration register*/
    __IO uint32_t  PCR_IISMode; /**< I2S protocol configuration register*/
    __IO uint32_t  PCR_SSCMode;	/**< SPI protocol configuration register*/
    __IO uint32_t  PCR;			/**< Protocol configuration register*/
    __IO uint32_t  PCR_ASCMode;	/**< UART protocol configuration register*/
  };
  __IO uint32_t  CCR;			/**< Channel control register*/
  __IO uint32_t  CMTR;			/**< Capture mode timer register*/

  union {
    __IO uint32_t  PSR_IICMode;	/**< I2C protocol status register*/
    __IO uint32_t  PSR_IISMode;	/**< I2S protocol status register*/
    __IO uint32_t  PSR_SSCMode;	/**< SPI protocol status register*/
    __IO uint32_t  PSR;			/**< Protocol status register*/
    __IO uint32_t  PSR_ASCMode;	/**< UART protocol status register*/
  };
  __O  uint32_t  PSCR;			/**< Protocol status clear register*/
  __I  uint32_t  RBUFSR;		/**< Receive buffer status register*/
  __I  uint32_t  RBUF;			/**< Receive buffer register*/
  __I  uint32_t  RBUFD;			/**< Debug mode receive buffer register*/
  __I  uint32_t  RBUF0;			/**< Receive buffer 0*/
  __I  uint32_t  RBUF1;			/**< Receive buffer 1*/
  __I  uint32_t  RBUF01SR;		/**< Receive buffer status register*/
  __O  uint32_t  FMR;			/**< Flag modification register*/
  __I  uint32_t  RESERVED2[5];
  __IO uint32_t  TBUF[32];		/**< Tranmsit buffer registers*/
  __IO uint32_t  BYP;			/**< FIFO bypass register*/
  __IO uint32_t  BYPCR;			/**< FIFO bypass control register*/
  __IO uint32_t  TBCTR;			/**< Transmit FIFO control register*/
  __IO uint32_t  RBCTR;			/**< Receive FIFO control register*/
  __I  uint32_t  TRBPTR;		/**< Transmit/recive buffer pointer register*/
  __IO uint32_t  TRBSR;			/**< Transmit/receive buffer status register*/
  __O  uint32_t  TRBSCR;		/**< Transmit/receive buffer status clear register*/
  __I  uint32_t  OUTR;			/**< Receive FIFO output register*/
  __I  uint32_t  OUTDR;			/**< Receive FIFO debug output register*/
  __I  uint32_t  RESERVED3[23];
  __O  uint32_t  IN[32];		/**< Transmit FIFO input register*/
} XMC_USIC_CH_t;


typedef struct {

  XMC_USIC_CH_t  *reg;
  void  (*set_baud_rate)(long val);
  void  (*send_byte)(char b);
  char  (*recv_byte)(long time);
  void  (*enable)(void);
  void  (*reset)(void);
  //void  (*enable_int)(void);
  void  (*rx_enable)(void);
  //void  (*rx_disable)(void);
  void  (*tx_enable)(void);
  //void  (*tx_disable)(void);
  //void  (*rx_enable_int)(void);
  //void  (*rx_disable_int)(void);
  //void  (*tx_enable_int)(void);
  //void  (*tx_disable_int)(void);
  void  (*set_mode)(void);
  //void  (*dbg_print)(char *string, long sz);
  void  (*init)(void);
  //long  (*write)(char *buff, long buff_sz);
  int  last_com_err;
  int  rx_complete;

} _US0;


void _UART_Init ();

void us0_open (void);
void us0_set_baud_rate (long val);
void us0_send_byte (char b);
char us0_recv_byte (long time);
void us0_enable (void);
void us0_reset (void);
void us0_init (void);
