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
 *  �����ж�ʹ��
 *  @param enable ʹ��״̬
 */
void HalInterruptsSetEnable(bool enable);

/**
 *  ��ȡ�ж�ʹ��״̬
 *  @return 0�رգ�1����
 */
bool HalInterruptsGetEnable(void);

/** �ر���־ */
void HalDisableLog(void);

/** ���� */
void HalReboot(void);

/** ϵͳ����ʱ�䣬���� */
uint32_t HalRunningTime(void);

/** ΢����ʱ */
void HalDelayUs(uint16_t us);

void HalEntrySleep(void);
void HalWakeup(void);
void HalStatusLedSet(uint8_t blinkCount);

#endif // HAL_H



