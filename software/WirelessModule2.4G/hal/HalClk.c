#include "HalClk.h"
#include "HalCtype.h"
#include "stm32f0xx_syscfg.h"
#include "HalUart.h"

//#include "stdio.h"
#define SYS_CLK_HSE
//#define SYS_CLK_HSI

//#define BOOT_WITH_IAP

#ifdef BOOT_WITH_IAP
#define APPLICATION_ADDRESS       (uint32_t)0x08000800  //(2 * 1024)

#if   (defined ( __CC_ARM ))
        __IO uint32_t VectorTable[48] __attribute__((at(0x20000000)));
#elif (defined (__ICCARM__))
#pragma location = 0x20000000
        __no_init __IO uint32_t VectorTable[48];
#elif defined   (  __GNUC__  )
        __IO uint32_t VectorTable[48] __attribute__((section(".RAMVectorTable")));
#elif defined ( __TASKING__ )
        __IO uint32_t VectorTable[48] __at(0x20000000);
#endif


#endif //DEBUG


static void HalInternalClkInit(void)
{
	uint32_t flagCount = 0;
    ErrorStatus startUpStatus;
	RCC_DeInit();
#if defined(SYS_CLK_HSI)
	RCC_HSICmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET)
	{
		flagCount++;
		if(flagCount > HSI_STARTUP_TIMEOUT)
		{
			break;
		}
	}
#else
    RCC_HSEConfig(RCC_HSE_ON);
    startUpStatus = RCC_WaitForHSEStartUp();
    if(startUpStatus != SUCCESS)
    {
    }
#endif	
	/* Enable Prefetch Buffer and set Flash Latency */
	FLASH_PrefetchBufferCmd(ENABLE);
	FLASH_SetLatency(FLASH_Latency_1); //flash wait time, 2 cpu cycle
	
	/* HCLK = SYSCLK */
	RCC_HCLKConfig(RCC_SYSCLK_Div1);
	
	/* PCLK = HCLK */
	RCC_PCLKConfig(RCC_HCLK_Div1); 
	
	RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);
	RCC_PLLCmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
	{

	}
	
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	while(RCC_GetSYSCLKSource() != RCC_CFGR_SWS_PLL)
	{

	}
}

void HalVectorTabInit(void)
{
#ifdef BOOT_WITH_IAP

    uint32_t i = 0;
	
    /* Relocate by software the vector table to the internal SRAM at 0x20000000 ***/      
    /* Copy the vector table from the Flash (mapped at the base of the application
        load address 0x08005000) to the base address of the SRAM at 0x20000000. */
    for(i = 0; i < 48; i++)
    {
        VectorTable[i] = *(__IO uint32_t*)(APPLICATION_ADDRESS + (i<<2));
    }

    /* Enable the SYSCFG peripheral clock*/
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SYSCFG, ENABLE); 
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    /* Remap SRAM at 0x00000000 */
    SYSCFG_MemoryRemapConfig(SYSCFG_MemoryRemap_SRAM);
#endif
}

void HalClkInit(void)
{
    HalVectorTabInit();
    HalInternalClkInit();
    
//	#if defined(SYS_CLK_HSE)
//	SystemInit();
//	#elif defined(SYS_CLK_HSI)
//	HalInternalClkInit();
//	#else
//	SystemInit();
//	#endif
    
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA|RCC_AHBPeriph_GPIOB, ENABLE);
//#ifdef HAL_UART_DMA
	//RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
//#endif
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);  
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);  
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);    //sysTime 
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE); 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);    //halWait
    
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); 

}



