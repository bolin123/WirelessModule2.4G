#ifndef HAL_POWER_H
#define HAL_POWER_H

#include "Sys.h"

void HalSetSleepPinStatus(bool sleep);
bool HalPowerNeedSleep(void);
void HalPowerSleep(void);
void HalPowerWakeup(void);
void HalPowerInitialize(void);
void HalPowerPoll(void);

#endif

