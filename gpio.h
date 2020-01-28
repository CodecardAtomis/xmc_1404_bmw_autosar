#include "XMC1400.h"

#define XMC_GPIO_OUTPUT_LEVEL_LOW       0x10000U /**<  Reset bit */
#define XMC_GPIO_OUTPUT_LEVEL_HIGH      0x1U 	/**< Set bit  */

#define PORT_IOCR_PC_Pos PORT0_IOCR0_PC0_Pos
#define PORT_IOCR_PC_Msk PORT0_IOCR0_PC0_Msk
#define PORT_IOCR_PC_Size 		(8U)
#define PORT_HWSEL_Msk                  PORT0_HWSEL_HW0_Msk
#define PORT_PHCR_Msk                   PORT0_PHCR0_PH0_Msk
#define PORT_PHCR_Size                  PORT0_PHCR0_PH0_Msk

#define PORT0_BASE                      0x40040000UL
#define PORT1_BASE                      0x40040100UL
#define PORT2_BASE                      0x40040200UL
#define PORT3_BASE                      0x40040300UL
#define PORT4_BASE                      0x40040400UL
/**********************************************************************************************************************
 * MACROS
 *********************************************************************************************************************/

#if defined(PORT0)
#define XMC_GPIO_PORT0 ((XMC_GPIO_PORT_t *) PORT0_BASE)
#define XMC_GPIO_CHECK_PORT0(port) (port == XMC_GPIO_PORT0)
#else
#define XMC_GPIO_CHECK_PORT0(port) 0
#endif

#if defined(PORT1)
#define XMC_GPIO_PORT1 ((XMC_GPIO_PORT_t *) PORT1_BASE)
#define XMC_GPIO_CHECK_PORT1(port) (port == XMC_GPIO_PORT1)
#else
#define XMC_GPIO_CHECK_PORT1(port) 0
#endif

#if defined(PORT2)
#define XMC_GPIO_PORT2 ((XMC_GPIO_PORT_t *) PORT2_BASE)
#define XMC_GPIO_CHECK_PORT2(port) (port == XMC_GPIO_PORT2)
#else
#define XMC_GPIO_CHECK_PORT2(port) 0
#endif

#if defined(PORT3)
#define XMC_GPIO_PORT3 ((XMC_GPIO_PORT_t *) PORT3_BASE)
#define XMC_GPIO_CHECK_PORT3(port) (port == XMC_GPIO_PORT3)
#else
#define XMC_GPIO_CHECK_PORT3(port) 0
#endif

#if defined(PORT4)
#define XMC_GPIO_PORT4 ((XMC_GPIO_PORT_t *) PORT4_BASE)
#define XMC_GPIO_CHECK_PORT4(port) (port == XMC_GPIO_PORT4)
#else
#define XMC_GPIO_CHECK_PORT4(port) 0
#endif

#define XMC_GPIO_CHECK_PORT(port) (XMC_GPIO_CHECK_PORT0(port) || \
                                   XMC_GPIO_CHECK_PORT1(port) || \
                                   XMC_GPIO_CHECK_PORT2(port) || \
                                   XMC_GPIO_CHECK_PORT3(port) || \
                                   XMC_GPIO_CHECK_PORT4(port))

#define XMC_GPIO_CHECK_OUTPUT_PORT(port) XMC_GPIO_CHECK_PORT(port)
                                   
#define XMC_GPIO_CHECK_ANALOG_PORT(port) (port == XMC_GPIO_PORT2)


typedef enum XMC_GPIO_MODE
{
  XMC_GPIO_MODE_INPUT_TRISTATE = 0x0UL << PORT_IOCR_PC_Pos,           /**< No internal pull device active */
  XMC_GPIO_MODE_INPUT_PULL_DOWN = 0x1UL << PORT_IOCR_PC_Pos,          /**< Internal pull-down device active */
  XMC_GPIO_MODE_INPUT_PULL_UP = 0x2UL << PORT_IOCR_PC_Pos,            /**< Internal pull-up device active */
  XMC_GPIO_MODE_INPUT_SAMPLING = 0x3UL << PORT_IOCR_PC_Pos,           /**< No internal pull device active; Pn_OUTx continuously samples the input value */
  XMC_GPIO_MODE_INPUT_INVERTED_TRISTATE = 0x4UL << PORT_IOCR_PC_Pos,  /**< Inverted no internal pull device active */
  XMC_GPIO_MODE_INPUT_INVERTED_PULL_DOWN = 0x5UL << PORT_IOCR_PC_Pos, /**< Inverted internal pull-down device active */
  XMC_GPIO_MODE_INPUT_INVERTED_PULL_UP = 0x6UL << PORT_IOCR_PC_Pos,   /**< Inverted internal pull-up device active */
  XMC_GPIO_MODE_INPUT_INVERTED_SAMPLING = 0x7UL << PORT_IOCR_PC_Pos,  /**< Inverted no internal pull device active;Pn_OUTx continuously samples the input value */
  XMC_GPIO_MODE_OUTPUT_PUSH_PULL = 0x80UL,		                      /**< Push-pull general-purpose output */
  XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN = 0xc0UL, 	                      /**< Open-drain general-purpose output */
  XMC_GPIO_MODE_OUTPUT_ALT1 = 0x1UL << PORT_IOCR_PC_Pos,
  XMC_GPIO_MODE_OUTPUT_ALT2 = 0x2UL << PORT_IOCR_PC_Pos,
  XMC_GPIO_MODE_OUTPUT_ALT3 = 0x3UL << PORT_IOCR_PC_Pos,
  XMC_GPIO_MODE_OUTPUT_ALT4 = 0x4UL << PORT_IOCR_PC_Pos,
  XMC_GPIO_MODE_OUTPUT_ALT5 = 0x5UL << PORT_IOCR_PC_Pos,
  XMC_GPIO_MODE_OUTPUT_ALT6 = 0x6UL << PORT_IOCR_PC_Pos,
  XMC_GPIO_MODE_OUTPUT_ALT7 = 0x7UL << PORT_IOCR_PC_Pos,
#if (UC_SERIES == XMC14)  
  XMC_GPIO_MODE_OUTPUT_ALT8 = 0x8UL << PORT_IOCR_PC_Pos,
  XMC_GPIO_MODE_OUTPUT_ALT9 = 0x9UL << PORT_IOCR_PC_Pos,
#endif  
  XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT1 = XMC_GPIO_MODE_OUTPUT_PUSH_PULL | XMC_GPIO_MODE_OUTPUT_ALT1, 	/**<  Push-pull alternate output function 1 */
  XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT2 = XMC_GPIO_MODE_OUTPUT_PUSH_PULL | XMC_GPIO_MODE_OUTPUT_ALT2, 	/**<  Push-pull alternate output function 2 */
  XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT3 = XMC_GPIO_MODE_OUTPUT_PUSH_PULL | XMC_GPIO_MODE_OUTPUT_ALT3, 	/**<  Push-pull alternate output function 3 */
  XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT4 = XMC_GPIO_MODE_OUTPUT_PUSH_PULL | XMC_GPIO_MODE_OUTPUT_ALT4, 	/**<  Push-pull alternate output function 4 */
  XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT5 = XMC_GPIO_MODE_OUTPUT_PUSH_PULL | XMC_GPIO_MODE_OUTPUT_ALT5, 	/**<  Push-pull alternate output function 5 */
  XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT6 = XMC_GPIO_MODE_OUTPUT_PUSH_PULL | XMC_GPIO_MODE_OUTPUT_ALT6, 	/**<  Push-pull alternate output function 6 */
  XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT7 = XMC_GPIO_MODE_OUTPUT_PUSH_PULL | XMC_GPIO_MODE_OUTPUT_ALT7, 	/**<  Push-pull alternate output function 7 */
#if (UC_SERIES == XMC14)  
  XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT8 = XMC_GPIO_MODE_OUTPUT_PUSH_PULL | XMC_GPIO_MODE_OUTPUT_ALT8, 	/**<  Push-pull alternate output function 8 */
  XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT9 = XMC_GPIO_MODE_OUTPUT_PUSH_PULL | XMC_GPIO_MODE_OUTPUT_ALT9, 	/**<  Push-pull alternate output function 9 */
#endif
  XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT1 = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN | XMC_GPIO_MODE_OUTPUT_ALT1, 	/**<  Open drain alternate output function 1 */
  XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT2 = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN | XMC_GPIO_MODE_OUTPUT_ALT2, 	/**<  Open drain alternate output function 2 */
  XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT3 = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN | XMC_GPIO_MODE_OUTPUT_ALT3, 	/**<  Open drain alternate output function 3 */
  XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT4 = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN | XMC_GPIO_MODE_OUTPUT_ALT4, 	/**<  Open drain alternate output function 4 */
  XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT5 = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN | XMC_GPIO_MODE_OUTPUT_ALT5, 	/**<  Open drain alternate output function 5 */
  XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT6 = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN | XMC_GPIO_MODE_OUTPUT_ALT6, 	/**<  Open drain alternate output function 6 */
  XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT7 = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN | XMC_GPIO_MODE_OUTPUT_ALT7, 	/**<  Open drain alternate output function 7 */
#if (UC_SERIES == XMC14)  
  XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT8 = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN | XMC_GPIO_MODE_OUTPUT_ALT8, 	/**<  Open drain alternate output function 8 */
  XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT9 = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN | XMC_GPIO_MODE_OUTPUT_ALT9 	/**<  Open drain alternate output function 9 */
#endif
} XMC_GPIO_MODE_t;


typedef struct XMC_GPIO_PORT {
  __IO uint32_t  OUT;				/**< The port output register determines the value of a GPIO pin when it is
  	  	  	  	  	  	  	  	  	  	 selected by Pn_IOCRx as output */
  __O  uint32_t  OMR;				/**< The port output modification register contains control bits that make it
  	  	  	  	  	  	  	  	  	  	 possible to individually set, reset, or toggle the logic state of a single port
  	  	  	  	  	  	  	  	  	     line*/
  __I  uint32_t  RESERVED0[2];
  __IO uint32_t  IOCR[4];			/**< The port input/output control registers select the digital output and input
  	  	  	  	  	  	  	  	  	  	 driver functionality and characteristics of a GPIO port pin */
  __I  uint32_t  RESERVED1;
  __I  uint32_t  IN;				/**< The logic level of a GPIO pin can be read via the read-only port input register
   	   	   	   	   	   	   	   	   	   	 Pn_IN */
  __I  uint32_t  RESERVED2[6];
  __IO uint32_t  PHCR[2];			/**< Pad hysteresis control register */
  __I  uint32_t  RESERVED3[6];
  __IO uint32_t  PDISC;				/**< Pin Function Decision Control Register is to disable/enable the digital pad
  	  	  	  	  	  	  	  	  	  	 structure in shared analog and digital ports*/
  __I  uint32_t  RESERVED4[3];
  __IO uint32_t  PPS;				/**< Pin Power Save Register */
  __IO uint32_t  HWSEL;				/**< Pin Hardware Select Register */
} XMC_GPIO_PORT_t;


//----------------------------
//   BASE = 0xFFFFF400
//----------------------------


typedef struct {

  XMC_GPIO_PORT_t  *reg;
  void  (*set_input)(char nr);
  void  (*set_output)(char nr);
  void  (*enable)(char nr);
  void  (*enable_pull_up)(char nr);
  void  (*disable_pull_up)(char nr);
  void  (*enable_filter)(char nr);
  void  (*disable_filter)(char nr);
  void  (*set_pin)(char nr, char bt);
  long  (*get_pin)(char nr);
  void  (*enable_int)(char nr);
  void  (*disable_int)(char nr);
  void  (*select_a)(char nr);
  void  (*select_b)(char nr);

} _GPIO_0;

typedef struct {

  XMC_GPIO_PORT_t  *reg;
  void  (*set_input)(char nr);
  void  (*set_output)(char nr);
  void  (*enable)(char nr);
  void  (*enable_pull_up)(char nr);
  void  (*disable_pull_up)(char nr);
  void  (*enable_filter)(char nr);
  void  (*disable_filter)(char nr);
  void  (*set_pin)(char nr, char bt);
  long  (*get_pin)(char nr);
  void  (*enable_int)(char nr);
  void  (*disable_int)(char nr);
  void  (*select_a)(char nr);
  void  (*select_b)(char nr);

} _GPIO_1;

typedef struct {

  XMC_GPIO_PORT_t  *reg;
  void  (*set_input)(char nr);
  void  (*set_output)(char nr);
  void  (*enable)(char nr);
  void  (*enable_pull_up)(char nr);
  void  (*disable_pull_up)(char nr);
  void  (*enable_filter)(char nr);
  void  (*disable_filter)(char nr);
  void  (*set_pin)(char nr, char bt);
  long  (*get_pin)(char nr);
  void  (*enable_int)(char nr);
  void  (*disable_int)(char nr);
  void  (*select_a)(char nr);
  void  (*select_b)(char nr);

} _GPIO_2;

typedef struct {

  XMC_GPIO_PORT_t  *reg;
  void  (*set_input)(char nr);
  void  (*set_output)(char nr);
  void  (*enable)(char nr);
  void  (*enable_pull_up)(char nr);
  void  (*disable_pull_up)(char nr);
  void  (*enable_filter)(char nr);
  void  (*disable_filter)(char nr);
  void  (*set_pin)(char nr, char bt);
  long  (*get_pin)(char nr);
  void  (*enable_int)(char nr);
  void  (*disable_int)(char nr);
  void  (*select_a)(char nr);
  void  (*select_b)(char nr);

} _GPIO_3;

typedef struct {

  XMC_GPIO_PORT_t  *reg;
  void  (*set_input)(char nr);
  void  (*set_output)(char nr);
  void  (*enable)(char nr);
  void  (*enable_pull_up)(char nr);
  void  (*disable_pull_up)(char nr);
  void  (*enable_filter)(char nr);
  void  (*disable_filter)(char nr);
  void  (*set_pin)(char nr, char bt);
  long  (*get_pin)(char nr);
  void  (*enable_int)(char nr);
  void  (*disable_int)(char nr);
  void  (*select_a)(char nr);
  void  (*select_b)(char nr);

} _GPIO_4;
//----------------------------
void gpio_0_set_input (char nr);
void gpio_0_set_output (char nr);
void gpio_0_enable (char nr);
void gpio_0_enable_pull_up (char nr);
void gpio_0_disable_pull_up (char nr);
void gpio_0_enable_filter (char nr);
void gpio_0_disable_filter (char nr);
void gpio_0_set_pin (char nr, char bt);
long gpio_0_get_pin (char nr);
void gpio_0_enable_int (char nr);
void gpio_0_disable_int (char nr);
void gpio_0_select_a (char nr);
void gpio_0_select_b (char nr);
void gpio_0_open (void);

void gpio_1_set_input (char nr);
void gpio_1_set_output (char nr);
void gpio_1_enable (char nr);
void gpio_1_enable_pull_up (char nr);
void gpio_1_disable_pull_up (char nr);
void gpio_1_enable_filter (char nr);
void gpio_1_disable_filter (char nr);
void gpio_1_set_pin (char nr, char bt);
long gpio_1_get_pin (char nr);
void gpio_1_enable_int (char nr);
void gpio_1_disable_int (char nr);
void gpio_1_select_a (char nr);
void gpio_1_select_b (char nr);
void gpio_1_open (void);

void gpio_2_set_input (char nr);
void gpio_2_set_output (char nr);
void gpio_2_enable (char nr);
void gpio_2_enable_pull_up (char nr);
void gpio_2_disable_pull_up (char nr);
void gpio_2_enable_filter (char nr);
void gpio_2_disable_filter (char nr);
void gpio_2_set_pin (char nr, char bt);
long gpio_2_get_pin (char nr);
void gpio_2_enable_int (char nr);
void gpio_2_disable_int (char nr);
void gpio_2_select_a (char nr);
void gpio_2_select_b (char nr);
void gpio_2_open (void);

void gpio_3_set_input (char nr);
void gpio_3_set_output (char nr);
void gpio_3_enable (char nr);
void gpio_3_enable_pull_up (char nr);
void gpio_3_disable_pull_up (char nr);
void gpio_3_enable_filter (char nr);
void gpio_3_disable_filter (char nr);
void gpio_3_set_pin (char nr, char bt);
long gpio_3_get_pin (char nr);
void gpio_3_enable_int (char nr);
void gpio_3_disable_int (char nr);
void gpio_3_select_a (char nr);
void gpio_3_select_b (char nr);
void gpio_3_open (void);

void gpio_4_set_input (char nr);
void gpio_4_set_output (char nr);
void gpio_4_enable (char nr);
void gpio_4_enable_pull_up (char nr);
void gpio_4_disable_pull_up (char nr);
void gpio_4_enable_filter (char nr);
void gpio_4_disable_filter (char nr);
void gpio_4_set_pin (char nr, char bt);
long gpio_4_get_pin (char nr);
void gpio_4_enable_int (char nr);
void gpio_4_disable_int (char nr);
void gpio_4_select_a (char nr);
void gpio_4_select_b (char nr);
void gpio_4_open (void);
