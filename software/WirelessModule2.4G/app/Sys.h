#ifndef SYS_H
#define SYS_H

#include "HalCtype.h"
#include "HalCommon.h"

typedef uint32_t SysTime_t;

#define SYS_NET_BUILD_INFO_ADDR  (0x8000000 + 0xF000)  //60k
#define SYS_DEVICE_INFO_ADDR     (0x8000000 + 0xF400)  //61k
#define SYS_DEVICE_MAC_ADDR      (0x8000000 + 0xF800)  //62k

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

void SysReboot(void);
void SysInitialize(void);
void SysPoll(void);

#endif

