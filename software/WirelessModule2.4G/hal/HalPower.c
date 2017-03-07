#include "HalPower.h"
#include "HalClk.h"
#include "HalGPIO.h"
#include "stm32f0xx_syscfg.h"
#include "stm32f0xx_pwr.h"

#define PM_WAKEUP_SET_PIN 0x00
#define PM_SLEEP_STATUS_PIN 0x08

static void wakeupIrqInit(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;  
    NVIC_InitTypeDef NVIC_InitStructure;  
        
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;    //设置引脚  
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; //设置为外部中断模式  
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  // 设置上升和下降沿都检测  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;                //使能外部中断新状态  
    EXTI_Init(&EXTI_InitStructure); 
    
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                //使能外部中断通道     
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_1_IRQn;  
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x00;         //先占优先级4位,共16级  
    NVIC_Init(&NVIC_InitStructure);   //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器  
}

extern void WMWakeupHandle(void);
void EXTI0_1_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line0) != RESET)  
    {
        WMWakeupHandle();
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

bool HalPowerNeedSleep(void)
{
    return (HalGPIOGet(PM_WAKEUP_SET_PIN) == HAL_GPIO_LEVEL_LOW);
}

void HalPowerSleep(void)
{
    HalGPIOSet(PM_SLEEP_STATUS_PIN, HAL_GPIO_LEVEL_LOW);
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
}

void HalPowerWakeup(void)
{
    HalClkInit(); //reinit system clock
    HalGPIOSet(PM_SLEEP_STATUS_PIN, HAL_GPIO_LEVEL_HIGH);
}

void HalPowerPoll(void)
{
    
}

void HalPowerInitialize(void)
{
    HalGPIOInit(PM_WAKEUP_SET_PIN, HAL_GPIO_DIR_IN);
    HalGPIOInit(PM_SLEEP_STATUS_PIN, HAL_GPIO_DIR_OUT);
    wakeupIrqInit();
    HalGPIOSet(PM_SLEEP_STATUS_PIN, HAL_GPIO_LEVEL_HIGH); //默认为唤醒状态
}

