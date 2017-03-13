#ifndef SYS_H
#define SYS_H

#include "HalCtype.h"
#include "HalCommon.h"

#define SYS_TEST_WM 0

#define SYS_UART_LOGS_PORT HAL_UART_1  //��־����
#define SYS_UART_COMM_PORT HAL_UART_0  //ͨ�Ŵ��� 

typedef uint32_t SysTime_t;

#define SYS_NET_BUILD_INFO_ADDR  (0x8000000 + 0xF000)  //60k ������Ϣ
#define SYS_DEVICE_INFO_ADDR     (0x8000000 + 0xF400)  //61k �����豸��Ϣ
#define SYS_DEVICE_MAC_ADDR      (0x8000000 + 0xF800)  //62k MAC��ַ

//extern uint32_t HalGetTimeCount(void);

#define SysTime HalRunningTime
#define SysTimeHasPast(lastTime, passTime) (SysTime() - (lastTime) > (passTime))

#define SYS_ENTER_CRITICAL(istate) //istate = HalInterruptsGetEnable();HalInterruptsSetEnable(0);
#define SYS_EXIT_CRITICAL(istate) //HalInterruptsSetEnable(istate);

bool SysDataSendListEmpty(void);
bool SysGotDeviceInfo(void);
uint8_t *SysGetVersion(void);
void SysRandomSeed(uint32_t seed);
uint32_t SysRandom(void);

uint32_t SysMacToUid(const uint8_t *mac);
uint8_t *SysGetMacAddr(uint8_t *mac);
void SysSetMacAddr(uint8_t *mac);
const uint8_t *SysUidToMac(uint32_t uid);

void SysReboot(void);
void SysInitialize(void);
void SysPoll(void);

#endif

