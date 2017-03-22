#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f0xx.h"

#if 0
/*Development Kit*/
#define HAL_STATUS_LED_PIN   0x11 //PB1
#define HAL_RF_CHIP_CS_PIN   0x04 //PA4
#define HAL_RF_CHIP_CE_PIN   0x10 //PB0
#define HAL_RF_CHIP_IRQ_PIN  0x01 //PA1
#else
/*Wireless Module*/
#define HAL_STATUS_LED_PIN   0x01 //PA1
#define HAL_RF_CHIP_CS_PIN   0x10 //PB0
#define HAL_RF_CHIP_CE_PIN   0x11 //PB1
#define HAL_RF_CHIP_IRQ_PIN  0x04 //PA4
#endif


#endif

