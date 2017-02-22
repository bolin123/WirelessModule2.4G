#ifndef HAL_H
#define HAL_H

#include "HalCtype.h"

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

/** 关闭日志 */
void HalDisableLog(void);

/** 重启 */
void HalReboot(void);

/** 系统运行时间，毫秒 */
uint32_t HalRunningTime(void);

/** 微秒延时 */
void HalDelayUs(uint16_t us);

#endif // HAL_H



