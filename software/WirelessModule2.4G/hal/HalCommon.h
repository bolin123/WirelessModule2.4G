#ifndef HAL_H
#define HAL_H

#include "HalClk.h"
#include "HalWdg.h"
#include "HalGPIO.h"
#include "HalTimer.h"
#include "HalUart.h"
#include "HalSpi.h"
#include "HalFlash.h"
#include "HalWait.h"

void HalInitialize(void);
void HalPoll(void);

/**
 *  设置中断使能
 *  @param enable 使能状态
 */
void HalInterruptsSetEnable(bool enable);

/**
 *  获取中断使能状态
 *  @return 0关闭，1开启
 */
bool HalInterruptsGetEnable(void);

/** 重启 */
void HalReboot(void);

/** 系统运行时间，毫秒 */
uint32_t HalRunningTime(void);

/*设置LED闪烁方式*/
void HalStatusLedSet(uint8_t blinkCount);

#endif // HAL_H



