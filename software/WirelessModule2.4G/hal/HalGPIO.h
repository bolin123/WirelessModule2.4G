#ifndef HALGPIO_H
#define HALGPIO_H

#include "HalCtype.h"

#define HAL_GPIO_NUM_INVAILD 0xff

typedef uint8_t HalGPIO_t;
//GPIO方向
typedef enum
{
    HAL_GPIO_DIR_IN,          // 输入
    HAL_GPIO_DIR_OUT,         // 输出
}HalGPIODir_t;

//IO口电平
typedef enum
{
    HAL_GPIO_LEVEL_LOW = 0,
    HAL_GPIO_LEVEL_HIGH = 1,
}HalGPIOLevel_t;

void HalGPIOInitialize(void);
void HalGPIOPoll(void);

/**
 *  GPIO 配置
 *  @param gpio 指定IO
 *  @param dir IO方向
 */
void HalGPIOInit(HalGPIO_t gpio, HalGPIODir_t dir);

/**
 *  设置IO的电平状态
 *  @param gpio 指定IO
 *  @param value 0低电平，1高电平
 */
void HalGPIOSet(HalGPIO_t gpio, HalGPIOLevel_t value);

/**
 *  获取IO电平状态
 *  @param HalGPIO_t 指定IO
 *  @return 0低电平，1高电平
 */
HalGPIOLevel_t HalGPIOGet(HalGPIO_t gpio);

#endif // HALGPIO_H






