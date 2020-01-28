#ifndef XMC1404_BOARD_INIT_H
#define XMC1404_BOARD_INIT_H

#include <XMC1400.h>		//SFR declarations of the selected device
#include <xmc_gpio.h>
#include <xmc1_gpio.h>
#include <xmc_can.h>

// PIN Definitions
#define LED0 P4_0
#define CAN1_TXD P4_9 //ALT9
#define CAN1_RXD P4_8

// Initialization Functions
void GPIO_Init();
void SCU_Init();
void CAN_Init();

// Global Functions
void CAN_Tx (void);

#endif //XMC1404_BOARD_INIT_H