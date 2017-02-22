#ifndef SYS_H
#define SYS_H

#include "HalCtype.h"
#include "HalCommon.h"

typedef uint32_t SysTime_t;

//extern uint32_t HalGetTimeCount(void);

#define SysTime HalRunningTime
#define SysTimeHasPast(lastTime, passTime) (SysTime() - (lastTime) > (passTime))

#define SYS_ENTER_CRITICAL(istate) //istate = HalInterruptsGetEnable();HalInterruptsSetEnable(0);
#define SYS_EXIT_CRITICAL(istate) //HalInterruptsSetEnable(istate);

uint32_t SysRandom(void);
void SysRandomSeed(uint32_t seed);

uint32_t SysMacToUid(const uint8_t *mac);
const uint8_t *SysUidToMac(uint32_t uid);
uint8_t *SysGetMacAddr(void);

void SysInitialize(void);
void SysPoll(void);

#endif

