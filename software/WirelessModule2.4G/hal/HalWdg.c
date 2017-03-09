#include "HalWdt.h"
#include "stm32f0xx_iwdg.h"

void HalWdgInitialize(void)
{
#if 0
	//检测系统是否由独立看门狗喂狗复位
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
	{ 
		//清除复位标志位 
		RCC_ClearFlag();
		printf("Watchdog reset!\n");
	}
	//使能写访问IWDG_PR and IWDG_RLR 寄存器
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	//IWDG计数器的时钟: LSI/32 
	IWDG_SetPrescaler(IWDG_Prescaler_64);

	IWDG_SetReload(0xEA6);//3s
	//Reload IWDG counter 
	IWDG_ReloadCounter();
	//Enable IWDG (the LSI oscillator will be enabled by hardware) 
	IWDG_Enable();
#endif
}

void HalWdgFeed(void)
{
#if 0
	/* 从新导入IWDG计数器 */
	IWDG_ReloadCounter();  
#endif
}
