#include "XMC1400.h"

#define SCU_GCU_PASSWD_PROT_ENABLE  (195UL) /**< Password for enabling protection */
#define SCU_GCU_PASSWD_PROT_DISABLE (192UL) /**< Password for disabling protection */

typedef struct {

  //Pmc  *reg;
  void  (*enable_peripheral_clock)(uint32_t per);
  void  (*disable_peripheral_clock)(uint32_t per);
  void  (*disable_all_peripherals)(void);

} _SCU;

void scu_open (void);
void scu_enable_peripheral_clock (uint32_t per);
void scu_disable_peripheral_clock (uint32_t per);
