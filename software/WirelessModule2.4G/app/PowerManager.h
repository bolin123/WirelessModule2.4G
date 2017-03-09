#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "Sys.h"

void PMSetSleepStatus(bool sleep);
void PMWakeUp(void);
void PMSleep(void);
void PMInitialize(void);
void PMPoll(void);

#endif

