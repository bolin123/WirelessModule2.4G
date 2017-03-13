#ifndef HALGPIO_H
#define HALGPIO_H

#include "HalCtype.h"

#define HAL_GPIO_NUM_INVAILD 0xff

typedef uint8_t HalGPIO_t;
//GPIO����
typedef enum
{
    HAL_GPIO_DIR_IN,          // ����
    HAL_GPIO_DIR_OUT,         // ���
}HalGPIODir_t;

//IO�ڵ�ƽ
typedef enum
{
    HAL_GPIO_LEVEL_LOW = 0,
    HAL_GPIO_LEVEL_HIGH = 1,
}HalGPIOLevel_t;

void HalGPIOInitialize(void);
void HalGPIOPoll(void);

/**
 *  GPIO ����
 *  @param gpio ָ��IO
 *  @param dir IO����
 */
void HalGPIOInit(HalGPIO_t gpio, HalGPIODir_t dir);

/**
 *  ����IO�ĵ�ƽ״̬
 *  @param gpio ָ��IO
 *  @param value 0�͵�ƽ��1�ߵ�ƽ
 */
void HalGPIOSet(HalGPIO_t gpio, HalGPIOLevel_t value);

/**
 *  ��ȡIO��ƽ״̬
 *  @param HalGPIO_t ָ��IO
 *  @return 0�͵�ƽ��1�ߵ�ƽ
 */
HalGPIOLevel_t HalGPIOGet(HalGPIO_t gpio);

#endif // HALGPIO_H






