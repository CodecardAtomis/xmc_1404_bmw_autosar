#include "XMC1400.h"
#include "gpio.h"

//----------------------------------------
_GPIO_0  *pgpio_0;
_GPIO_0   pgpio_0_buffer;

_GPIO_1  *pgpio_1;
_GPIO_1   pgpio_1_buffer;

_GPIO_2  *pgpio_2;
_GPIO_2   pgpio_2_buffer;

_GPIO_3  *pgpio_3;
_GPIO_3   pgpio_3_buffer;

_GPIO_4  *pgpio_4;
_GPIO_4   pgpio_4_buffer;
//----------------------------------------
//              PORT 0
//----------------------------------------
void gpio_0_set_input (char nr)
{
  /* Switch to input */
  pgpio_0->reg->IOCR[nr >> 2U] &= ~(uint32_t)((uint32_t)PORT_IOCR_PC_Msk << (PORT_IOCR_PC_Size * (nr & 0x3U)));

  /* HW port control is disabled */
  //pgpio_0->reg->HWSEL &= ~(uint32_t)((uint32_t)PORT_HWSEL_Msk << ((uint32_t)nr << 1U));
  
  /* Set input hysteresis */
  pgpio_0->reg->PHCR[(uint32_t)nr >> 3U] &= ~(uint32_t)((uint32_t)PORT_PHCR_Msk << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U))); 
  
  //pgpio_0->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
} 

long gpio_0_get_pin (char nr)
{
  uint32_t val = pgpio_0->reg->IN;
  
  if ((val & (1 << nr)) != 0)
      return 1;
    
  return 0;
}

void gpio_0_set_output (char nr)
{
  /* Switch to input */
  pgpio_0->reg->IOCR[nr >> 2U] &= ~(uint32_t)((uint32_t)PORT_IOCR_PC_Msk << (PORT_IOCR_PC_Size * (nr & 0x3U)));

  /* HW port control is disabled */
  pgpio_0->reg->HWSEL &= ~(uint32_t)((uint32_t)PORT_HWSEL_Msk << ((uint32_t)nr << 1U));

  /* Set input hysteresis */
  pgpio_0->reg->PHCR[(uint32_t)nr >> 3U] &= ~(uint32_t)((uint32_t)PORT_PHCR_Msk << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U)));
  //pgpio->reg->PHCR[(uint32_t)nr >> 3U] |= (uint32_t)config->input_hysteresis << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U));
    
  /* Enable digital input */
  //if (XMC_GPIO_CHECK_ANALOG_PORT(port))
  //{    
  //  pgpio->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
  //}
  
  /* Set output level */
  pgpio_0->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_HIGH << nr;
  
  /* Set mode */
  pgpio_0->reg->IOCR[nr >> 2U] |= (uint32_t)XMC_GPIO_MODE_OUTPUT_PUSH_PULL << (PORT_IOCR_PC_Size * (nr & 0x3U));
} 

void gpio_0_set_pin (char nr, char bt)
{ 
  if (bt != 0)
    pgpio_0->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_HIGH << nr;
  else
    pgpio_0->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_LOW << nr;
}

/*const XMC_GPIO_CONFIG_t LED_config =
{
  .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
  .output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH
};*/

//-------------------------------------------
void gpio_0_open (void)
{
  pgpio_0 = (_GPIO_0 *)&pgpio_0_buffer;
  pgpio_0->reg = (XMC_GPIO_PORT_t *)PORT0_BASE;
  pgpio_0->set_input = gpio_0_set_input;
  pgpio_0->set_output = gpio_0_set_output;
  //pgpio->enable = gpio_enable;
  //pgpio->enable_pull_up = pioa_enable_pull_up;
  //pgpio->disable_pull_up = pioa_disable_pull_up;
  //pgpio->enable_filter = pioa_enable_filter;
  //pgpio->disable_filter = pioa_disable_filter;
  pgpio_0->set_pin = gpio_0_set_pin;
  pgpio_0->get_pin = gpio_0_get_pin;
  //pgpio->get_pin = pioa_get_pin;
  //pgpio->enable_int = pioa_enable_int;
  //pgpio->disable_int = pioa_disable_int;
  //pgpio->select_a = pioa_select_a;
  //pgpio->select_b = pioa_select_b;
}

//----------------------------------------
//              PORT 1
//----------------------------------------
void gpio_1_set_input (char nr)
{
  /* Switch to input */
  pgpio_1->reg->IOCR[nr >> 2U] &= ~(uint32_t)((uint32_t)PORT_IOCR_PC_Msk << (PORT_IOCR_PC_Size * (nr & 0x3U)));

  /* HW port control is disabled */
  //pgpio_1->reg->HWSEL &= ~(uint32_t)((uint32_t)PORT_HWSEL_Msk << ((uint32_t)nr << 1U));
  
  /* Set input hysteresis */
  pgpio_1->reg->PHCR[(uint32_t)nr >> 3U] &= ~(uint32_t)((uint32_t)PORT_PHCR_Msk << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U))); 
  
  //pgpio_1->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
} 

long gpio_1_get_pin (char nr)
{
  uint32_t val = pgpio_1->reg->IN;
  
  if ((val & (1 << nr)) != 0)
      return 1;
    
  return 0;
}

void gpio_1_set_output (char nr)
{
  /* Switch to input */
  pgpio_1->reg->IOCR[nr >> 2U] &= ~(uint32_t)((uint32_t)PORT_IOCR_PC_Msk << (PORT_IOCR_PC_Size * (nr & 0x3U)));

  /* HW port control is disabled */
  //pgpio_1->reg->HWSEL &= ~(uint32_t)((uint32_t)PORT_HWSEL_Msk << ((uint32_t)nr << 1U));

  /* Set input hysteresis */
  pgpio_1->reg->PHCR[(uint32_t)nr >> 3U] &= ~(uint32_t)((uint32_t)PORT_PHCR_Msk << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U)));
  //pgpio->reg->PHCR[(uint32_t)nr >> 3U] |= (uint32_t)config->input_hysteresis << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U));
    
  /* Enable digital input */
  //if (XMC_GPIO_CHECK_ANALOG_PORT(port))
  //{    
  //  pgpio->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
  //}
  
  /* Set output level */
  pgpio_1->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_HIGH << nr;
  
  /* Set mode */
  pgpio_1->reg->IOCR[nr >> 2U] |= (uint32_t)XMC_GPIO_MODE_OUTPUT_PUSH_PULL << (PORT_IOCR_PC_Size * (nr & 0x3U));
} 

void gpio_1_set_pin (char nr, char bt)
{ 
  if (bt != 0)
    pgpio_1->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_HIGH << nr;
  else
    pgpio_1->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_LOW << nr;
}

/*const XMC_GPIO_CONFIG_t LED_config =
{
  .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
  .output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH
};*/

//-------------------------------------------
void gpio_1_open (void)
{
  pgpio_1 = (_GPIO_1 *)&pgpio_1_buffer;
  pgpio_1->reg = (XMC_GPIO_PORT_t *)PORT1_BASE;
  pgpio_1->set_input = gpio_1_set_input;
  pgpio_1->set_output = gpio_1_set_output;
  //pgpio->enable = gpio_enable;
  //pgpio->enable_pull_up = pioa_enable_pull_up;
  //pgpio->disable_pull_up = pioa_disable_pull_up;
  //pgpio->enable_filter = pioa_enable_filter;
  //pgpio->disable_filter = pioa_disable_filter;
  pgpio_1->set_pin = gpio_1_set_pin;
  pgpio_1->get_pin = gpio_1_get_pin;
  //pgpio->get_pin = pioa_get_pin;
  //pgpio->enable_int = pioa_enable_int;
  //pgpio->disable_int = pioa_disable_int;
  //pgpio->select_a = pioa_select_a;
  //pgpio->select_b = pioa_select_b;
}

//----------------------------------------
//              PORT 2
//----------------------------------------
void gpio_2_set_input (char nr)
{
  /* Switch to input */
  pgpio_2->reg->IOCR[nr >> 2U] &= ~(uint32_t)((uint32_t)PORT_IOCR_PC_Msk << (PORT_IOCR_PC_Size * (nr & 0x3U)));

  /* HW port control is disabled */
  //pgpio_2->reg->HWSEL &= ~(uint32_t)((uint32_t)PORT_HWSEL_Msk << ((uint32_t)nr << 1U));
  
  /* Enable digital input */
  pgpio_2->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);  
  
  /* Set input hysteresis */
  pgpio_2->reg->PHCR[(uint32_t)nr >> 3U] &= ~(uint32_t)((uint32_t)PORT_PHCR_Msk << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U))); 
  
  //pgpio_2->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
} 

long gpio_2_get_pin (char nr)
{
  uint32_t val = pgpio_2->reg->IN;
  
  if ((val & (1 << nr)) != 0)
      return 1;
    
  return 0;
}

void gpio_2_set_output (char nr)
{
  /* Switch to input */
  pgpio_2->reg->IOCR[nr >> 2U] &= ~(uint32_t)((uint32_t)PORT_IOCR_PC_Msk << (PORT_IOCR_PC_Size * (nr & 0x3U)));

  /* HW port control is disabled */
  pgpio_2->reg->HWSEL &= ~(uint32_t)((uint32_t)PORT_HWSEL_Msk << ((uint32_t)nr << 1U));

  /* Set input hysteresis */
  pgpio_2->reg->PHCR[(uint32_t)nr >> 3U] &= ~(uint32_t)((uint32_t)PORT_PHCR_Msk << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U)));
  //pgpio->reg->PHCR[(uint32_t)nr >> 3U] |= (uint32_t)config->input_hysteresis << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U));
    
  /* Enable digital input */
  pgpio_2->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
  
  //if (XMC_GPIO_CHECK_ANALOG_PORT(port))
  //{    
  //  pgpio->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
  //}
  
  /* Set output level */
  pgpio_2->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_HIGH << nr;
  
  /* Set mode */
  pgpio_2->reg->IOCR[nr >> 2U] |= (uint32_t)XMC_GPIO_MODE_OUTPUT_PUSH_PULL << (PORT_IOCR_PC_Size * (nr & 0x3U));
} 

void gpio_2_set_pin (char nr, char bt)
{ 
  if (bt != 0) {
    pgpio_2->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_HIGH << nr;
    pgpio_2->reg->OUT |= (1 << nr);
  }
  else {
    pgpio_2->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_LOW << nr;
    pgpio_2->reg->OUT &= ~(1 << nr);
  }
}

/*const XMC_GPIO_CONFIG_t LED_config =
{
  .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
  .output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH
};*/

//-------------------------------------------
void gpio_2_open (void)
{
  pgpio_2 = (_GPIO_2 *)&pgpio_2_buffer;
  pgpio_2->reg = (XMC_GPIO_PORT_t *)PORT2_BASE;
  pgpio_2->set_input = gpio_2_set_input;
  pgpio_2->set_output = gpio_2_set_output;
  //pgpio->enable = gpio_enable;
  //pgpio->enable_pull_up = pioa_enable_pull_up;
  //pgpio->disable_pull_up = pioa_disable_pull_up;
  //pgpio->enable_filter = pioa_enable_filter;
  //pgpio->disable_filter = pioa_disable_filter;
  pgpio_2->set_pin = gpio_2_set_pin;
  pgpio_2->get_pin = gpio_2_get_pin;
  //pgpio->get_pin = pioa_get_pin;
  //pgpio->enable_int = pioa_enable_int;
  //pgpio->disable_int = pioa_disable_int;
  //pgpio->select_a = pioa_select_a;
  //pgpio->select_b = pioa_select_b;
}

//----------------------------------------
//              PORT 3
//----------------------------------------
void gpio_3_set_input (char nr)
{
  /* Switch to input */
  pgpio_3->reg->IOCR[nr >> 2U] &= ~(uint32_t)((uint32_t)PORT_IOCR_PC_Msk << (PORT_IOCR_PC_Size * (nr & 0x3U)));

  /* HW port control is disabled */
  //pgpio_3->reg->HWSEL &= ~(uint32_t)((uint32_t)PORT_HWSEL_Msk << ((uint32_t)nr << 1U));
  
  /* Set input hysteresis */
  pgpio_3->reg->PHCR[(uint32_t)nr >> 3U] &= ~(uint32_t)((uint32_t)PORT_PHCR_Msk << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U))); 
  
  //pgpio_3->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
} 

long gpio_3_get_pin (char nr)
{
  uint32_t val = pgpio_3->reg->IN;
  
  if ((val & (1 << nr)) != 0)
      return 1;
    
  return 0;
} 

void gpio_3_set_output (char nr)
{
  /* Switch to input */
  pgpio_3->reg->IOCR[nr >> 2U] &= ~(uint32_t)((uint32_t)PORT_IOCR_PC_Msk << (PORT_IOCR_PC_Size * (nr & 0x3U)));

  /* HW port control is disabled */
  pgpio_3->reg->HWSEL &= ~(uint32_t)((uint32_t)PORT_HWSEL_Msk << ((uint32_t)nr << 1U));

  /* Set input hysteresis */
  pgpio_3->reg->PHCR[(uint32_t)nr >> 3U] &= ~(uint32_t)((uint32_t)PORT_PHCR_Msk << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U)));
  //pgpio->reg->PHCR[(uint32_t)nr >> 3U] |= (uint32_t)config->input_hysteresis << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U));
    
  /* Enable digital input */
  //if (XMC_GPIO_CHECK_ANALOG_PORT(port))
  //{    
  //  pgpio->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
  //}
  
  /* Set output level */
  pgpio_3->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_HIGH << nr;
  
  /* Set mode */
  pgpio_3->reg->IOCR[nr >> 2U] |= (uint32_t)XMC_GPIO_MODE_OUTPUT_PUSH_PULL << (PORT_IOCR_PC_Size * (nr & 0x3U));
} 

void gpio_3_set_pin (char nr, char bt)
{ 
  if (bt != 0)
    pgpio_3->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_HIGH << nr;
  else
    pgpio_3->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_LOW << nr;
}

/*const XMC_GPIO_CONFIG_t LED_config =
{
  .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
  .output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH
};*/

//-------------------------------------------
void gpio_3_open (void)
{
  pgpio_3 = (_GPIO_3 *)&pgpio_3_buffer;
  pgpio_3->reg = (XMC_GPIO_PORT_t *)PORT3_BASE;
  pgpio_3->set_input = gpio_3_set_input;
  pgpio_3->set_output = gpio_3_set_output;
  //pgpio->enable = gpio_enable;
  //pgpio->enable_pull_up = pioa_enable_pull_up;
  //pgpio->disable_pull_up = pioa_disable_pull_up;
  //pgpio->enable_filter = pioa_enable_filter;
  //pgpio->disable_filter = pioa_disable_filter;
  pgpio_3->set_pin = gpio_3_set_pin;
  pgpio_3->get_pin = gpio_3_get_pin;
  //pgpio->enable_int = pioa_enable_int;
  //pgpio->disable_int = pioa_disable_int;
  //pgpio->select_a = pioa_select_a;
  //pgpio->select_b = pioa_select_b;
}

//----------------------------------------
//              PORT 4
//----------------------------------------
void gpio_4_set_input (char nr)
{
  /* Switch to input */
  pgpio_4->reg->IOCR[nr >> 2U] &= ~(uint32_t)((uint32_t)PORT_IOCR_PC_Msk << (PORT_IOCR_PC_Size * (nr & 0x3U)));

  /* HW port control is disabled */
  //pgpio_4->reg->HWSEL &= ~(uint32_t)((uint32_t)PORT_HWSEL_Msk << ((uint32_t)nr << 1U));
  
  /* Set input hysteresis */
  pgpio_4->reg->PHCR[(uint32_t)nr >> 3U] &= ~(uint32_t)((uint32_t)PORT_PHCR_Msk << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U))); 
  
  //pgpio_4->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
} 

long gpio_4_get_pin (char nr)
{
  uint32_t val = pgpio_4->reg->IN;
  
  if ((val & (1 << nr)) != 0)
      return 1;
    
  return 0;
}

void gpio_4_set_output (char nr)
{
  /* Switch to input */
  pgpio_4->reg->IOCR[nr >> 2U] &= ~(uint32_t)((uint32_t)PORT_IOCR_PC_Msk << (PORT_IOCR_PC_Size * (nr & 0x3U)));

  /* HW port control is disabled */
  pgpio_4->reg->HWSEL &= ~(uint32_t)((uint32_t)PORT_HWSEL_Msk << ((uint32_t)nr << 1U));

  /* Set input hysteresis */
  pgpio_4->reg->PHCR[(uint32_t)nr >> 3U] &= ~(uint32_t)((uint32_t)PORT_PHCR_Msk << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U)));
  //pgpio->reg->PHCR[(uint32_t)nr >> 3U] |= (uint32_t)config->input_hysteresis << ((uint32_t)PORT_PHCR_Size * ((uint32_t)nr & 0x7U));
    
  /* Enable digital input */
  //if (XMC_GPIO_CHECK_ANALOG_PORT(port))
  //{    
  //  pgpio->reg->PDISC &= ~(uint32_t)((uint32_t)0x1U << nr);
  //}
  
  /* Set output level */
  pgpio_4->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_HIGH << nr;
  
  /* Set mode */
  pgpio_4->reg->IOCR[nr >> 2U] |= (uint32_t)XMC_GPIO_MODE_OUTPUT_PUSH_PULL << (PORT_IOCR_PC_Size * (nr & 0x3U));
} 

void gpio_4_set_pin (char nr, char bt)
{ 
  if (bt != 0)
    pgpio_4->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_HIGH << nr;
  else
    pgpio_4->reg->OMR = (uint32_t)XMC_GPIO_OUTPUT_LEVEL_LOW << nr;
}

/*const XMC_GPIO_CONFIG_t LED_config =
{
  .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
  .output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH
};*/

//-------------------------------------------
void gpio_4_open (void)
{
  pgpio_4 = (_GPIO_4 *)&pgpio_4_buffer;
  pgpio_4->reg = (XMC_GPIO_PORT_t *)PORT4_BASE;
  pgpio_4->set_input = gpio_4_set_input;
  pgpio_4->set_output = gpio_4_set_output;
  //pgpio->enable = gpio_enable;
  //pgpio->enable_pull_up = pioa_enable_pull_up;
  //pgpio->disable_pull_up = pioa_disable_pull_up;
  //pgpio->enable_filter = pioa_enable_filter;
  //pgpio->disable_filter = pioa_disable_filter;
  pgpio_4->set_pin = gpio_4_set_pin;
  pgpio_4->get_pin = gpio_4_get_pin;
  //pgpio->get_pin = pioa_get_pin;
  //pgpio->enable_int = pioa_enable_int;
  //pgpio->disable_int = pioa_disable_int;
  //pgpio->select_a = pioa_select_a;
  //pgpio->select_b = pioa_select_b;
}
