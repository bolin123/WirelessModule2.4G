#include "HalWait.h"

static uint8_t g_waitType;

void TIM1_Init(void);
void TIM1_Config_Us(void);
void TIM1_Config_Ms(void);


void hal_wait_init(uint8_t type)
{
	g_waitType = type;
	TIM1_Init();
	if(type == HAL_WAIT_US)
	{
		TIM1_Config_Us();
	}
	if(type == HAL_WAIT_MS)
	{
		TIM1_Config_Ms();
	}
}

void hal_wait_check(uint8_t type)
{
  if(type != g_waitType)
  {
    hal_wait_init(type);
  }
}

void hal_wait(uint16_t utime, uint8_t type)
{
	uint16_t count = 0;
	while(count < utime)
	{
		hal_wait_check(type);
		HalIwdtFeed();
	if((TIM1->SR)&TIM_IT_Update)
	{
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
		count++;
	}
	}
}

void hal_wait_us(uint16_t usec)
{
  hal_wait_init(HAL_WAIT_US);
  hal_wait(usec, HAL_WAIT_US);
}

void hal_wait_ms(uint16_t msec)
{
  hal_wait_init(HAL_WAIT_MS);
  hal_wait(msec, HAL_WAIT_MS);
}


void TIM1_Init(void)
{
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM1, ENABLE);
}
void TIM1_Config_Us(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = (48 - 1);
	TIM_TimeBaseStructure.TIM_Prescaler = (1 - 1);
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	// Clear TIM14 update pending flag[??TIM14??????] 
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	// TIM IT enable 
	//TIM_ITConfig(TIM14, TIM_IT_Update, ENABLE);
	// TIM14 enable counter 
	TIM_Cmd(TIM1, ENABLE);
}
void TIM1_Config_Ms(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = (1000 - 1);
	TIM_TimeBaseStructure.TIM_Prescaler = (48 - 1);
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	// Clear TIM14 update pending flag[??TIM14??????] 
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	// TIM IT enable 
	//TIM_ITConfig(TIM14, TIM_IT_Update, ENABLE);
	// TIM14 enable counter 
	TIM_Cmd(TIM1, ENABLE);
}
