#include "HalTimer.h"
#include "HalCtype.h"

typedef enum
{
    DIVDED_BY_1 = 0,
    DIVDED_BY_16 = 4,
    DIVDED_BY_256 = 8,
}TIMER_PREDIVED_MODE;

typedef enum
{
    TM_LEVEL_INT = 1,
    TM_EDGE_INT  = 0,
}TIMER_INT_MODE;

#define FRC1_ENABLE_TIMER   BIT7
#define FRC1_AUTO_RELOAD    BIT6


//HalTimerPassMsHandle g_passMsHandle;


static void timer3IrqConfig(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 2; //timer 1=1 ,timer 2=2 , timer3=3 ,timer4=4
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

static void timer3Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	timer3IrqConfig();

	TIM_TimeBaseStructure.TIM_Period = (1000 - 1);
	TIM_TimeBaseStructure.TIM_Prescaler = (48 - 1); //1ms
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	// TIM IT enable 
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	// TIM3 enable counter 
	TIM_Cmd(TIM3, ENABLE);
}

ROM_FUNC void HalTimerInitialize(void)
{
	//g_passMsHandle = msHandle;
	timer3Init();
}

extern void HalTimerPassMs(void);
void TIM3_IRQHandler(void)
{
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	HalTimerPassMs();
}


void HalWaitUs(uint32_t us)
{
	uint32_t temp;
	uint8_t radix = 48-1;
	
	
	SysTick->LOAD = us*radix;
	SysTick->VAL=0x00;
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; 
	
	do  
    {  
        temp = SysTick->CTRL;  
    }  
    while(temp&0x01&&!(temp&(1<<16)));
	
	SysTick->CTRL &= 0xfffffff8;  
    SysTick->VAL = 0x00;  
}
#if 0
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = (48 - 1)*us;
	TIM_TimeBaseStructure.TIM_Prescaler = (1 - 1);
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM14, &TIM_TimeBaseStructure);

	TIM_ClearITPendingBit(TIM14, TIM_IT_Update);

	TIM_Cmd(TIM14, ENABLE);

	while(TIM_GetFlagStatus(TIM14, TIM_FLAG_Update) != SET);
	TIM_ClearFlag(TIM14, TIM_FLAG_Update);
	
	TIM_Cmd(TIM14, DISABLE);
}
#endif



