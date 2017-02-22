#ifndef HAL_H
#define HAL_H

#include "HalCtype.h"

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

#endif // HAL_H



