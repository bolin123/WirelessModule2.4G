#ifndef HAL_SPI_H
#define HAL_SPI_H

#include "HalCtype.h"

void HalSpiInitialize(void);
void HalSpiPoll(void);
unsigned char HalSpiReadWrite(unsigned char data);

#endif

