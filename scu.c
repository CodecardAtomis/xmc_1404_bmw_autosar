#include "XMC1400.h"
#include "scu.h"

//----------------------------------------
_SCU  *pscu;
_SCU   pscu_buffer;
//----------------------------------------

void scu_UnlockProtectedBits(void)
{
  SCU_GENERAL->PASSWD = SCU_GCU_PASSWD_PROT_DISABLE;

  while(((SCU_GENERAL->PASSWD) & SCU_GENERAL_PASSWD_PROTS_Msk))
  {
    /* Loop until the lock is removed */
  }
}

void scu_LockProtectedBits(void)
{
  SCU_GENERAL->PASSWD = SCU_GCU_PASSWD_PROT_ENABLE;
}

//----------------------------------------------
void scu_enable_peripheral_clock (uint32_t per)
{  
  scu_UnlockProtectedBits();
  
  SCU_CLK->CGATCLR0 |= (uint32_t)per;
  while ((SCU_CLK->CLKCR) & SCU_CLK_CLKCR_VDDC2LOW_Msk)
  {
    /* Wait voltage suply stabilization */
  }  
  
  scu_LockProtectedBits();
}

void scu_disable_peripheral_clock (uint32_t per)
{
  scu_UnlockProtectedBits();
  SCU_CLK->CGATSET0 |= (uint32_t)per;
  scu_LockProtectedBits();
}

/*void scu_disable_all_peripherals (void)
{
  ppmc->reg->PMC_PCDR = 0xFFFFFFFF;
}*/
//--------------------------------------------
void scu_open (void)
{
  pscu = (_SCU *)&pscu_buffer;
  //pscu->reg = (Pmc *)0xFFFFFC00;
  pscu->enable_peripheral_clock = scu_enable_peripheral_clock;
  pscu->disable_peripheral_clock = scu_disable_peripheral_clock;
  //pscu->disable_all_peripherals = scu_disable_all_peripherals;
}
