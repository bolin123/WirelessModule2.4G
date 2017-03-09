#include "testWM.h"
#include "HalGPIO.h"
#include "WirelessModule.h"

#define DEV_MASTER  0

#define SLAVE_TYPE "SLV201"
#define MASTER_TYPE "MST001"
#define STATUS_LED_PIN 0x11  //PB1

static bool g_cooBuild = false;
static uint8_t g_cooAddress;

static void masterPoll(void)
{
#if 1
    static SysTime_t lastTime = 0;
    static bool isDeleted = false;
    uint8_t delAddr[1];

    if(lastTime == 0 || SysTimeHasPast(lastTime, 60000))
    {
        WMNetBuildSearch();
        lastTime = SysTime();
        
    }
/*
    if(SysTime() > 90000 && !isDeleted)
    {
        delAddr[0] = 1;
        WMNetBuildDelDevice(delAddr, sizeof(delAddr));
        isDeleted = true;
    }
*/
#endif
}
#include "stm32f0xx_pwr.h"
static void slavePoll(void)
{
    static SysTime_t lastTime;
    static uint8_t count = 0;
    uint8_t data[20] = {1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0};
    
    if(WMGetNetStatus() == WM_STATUS_NET_BUILDED && SysTimeHasPast(lastTime, 5000))
    {
        SysLog("send data:");
        memset(data, count, sizeof(data));
        WMNetUserDataSend(WM_MASTER_NET_ADDR, data, sizeof(data));

        if(g_cooBuild)
        {
            WMNetUserDataSend(g_cooAddress, data, sizeof(data));
        }
        lastTime = SysTime();
        count++;
    }
#if 0
    if(firstSleep && SysTimeHasPast(sleepTime, 10000))
    {
        firstSleep = false;
        lastTime = SysTime();
        HalGPIOSet(STATUS_LED_PIN, HAL_GPIO_LEVEL_HIGH);
        SysLog("sleeeeeeep....\n");
        NetLayerSleep(true);
        PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
    }
    
    static uint8_t count = 0;
    uint8_t data[20] = {1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0};
    
    if(WMGetNetStatus() == WM_STATUS_NET_BUILDED && SysTimeHasPast(lastTime, 3000))
    {
        SysLog("send data:");
        memset(data, count, sizeof(data));
        WMNetUserDataSend(WM_MASTER_NET_ADDR, data, sizeof(data));
        lastTime = SysTime();
        count++;
    }
#endif
}

static void testEventHandle(WMEvent_t event, void *args)
{
    uint8_t i, num;
    WMSearchResult_t result[WM_SEARCH_DEVICE_NUM];
    uint8_t devNum[WM_SEARCH_DEVICE_NUM];
    WMCoordinationData_t *cooData = NULL;

    SysLog("event %d", event);
    
    if(WM_EVENT_SEARCH_END == event)
    {
    #if 0
        num = WMGetSearchResult(result);
        if(num > 0)
        {
            for(i = 0; i < num; i++)
            {
                SysLog("sid:%d, sleep:%d, type:%c%c%c%c%c%c", result[i].sid, result[i].sleep, \
                    result[i].type[0], result[i].type[1], result[i].type[2], result[i].type[3], \
                    result[i].type[4], result[i].type[5]);
                devNum[i] = result[i].sid;
            }
            WMNetBuildAddDevice(devNum, num);
        }
    #endif
    }
    else if(WM_EVENT_NET_STATUS_CHANGE == event)
    {
        WMNetStatus_t  netStatus = (WMNetStatus_t)args;
        
        if(WM_STATUS_NET_IDLE == netStatus)
        {
            //HalGPIOSet(STATUS_LED_PIN, HAL_GPIO_LEVEL_HIGH);
        }
        else if(WM_STATUS_NET_BUILDING == netStatus)
        {
        }
        else //WM_STATUS_NET_BUILDED
        {
            //HalGPIOSet(STATUS_LED_PIN, HAL_GPIO_LEVEL_LOW);
        }
    }
    else if(WM_EVENT_RECV_SEARCH == event)
    {
        
    }
    else if(WM_EVENT_COORDINATION == event)
    {
        cooData = (WMCoordinationData_t *)args;
        if(cooData->build)
        {
            g_cooBuild = true;
            g_cooAddress = cooData->address;
        }
    }
    
}
/*
extern void HalClkInit(void);
void EXTI0_1_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line0) != RESET)  
    {
        HalClkInit();
        SysPrintf("PA0 Pin IRQ...\n");
        NetLayerSleep(false);
        HalGPIOSet(STATUS_LED_PIN, HAL_GPIO_LEVEL_LOW);
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

#include "stm32f0xx_syscfg.h"
static void irqInit(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;  
    NVIC_InitTypeDef NVIC_InitStructure;  

    HalGPIOInit(0x00, HAL_GPIO_DIR_IN);
        
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
*/
void testWMInit(void)
{
//    HalGPIOInit(STATUS_LED_PIN, HAL_GPIO_DIR_OUT);
//    HalGPIOSet(STATUS_LED_PIN, HAL_GPIO_LEVEL_LOW);
//    irqInit();
    WMInitialize();
    WMEventRegister(testEventHandle);
#if DEV_MASTER
    WMSetMasterSlaveMode(true);
    WMSetSleepMode(0);
    WMSetModuleType(MASTER_TYPE);
#else
    WMSetMasterSlaveMode(false);
    WMSetSleepMode(0);
    WMSetModuleType(SLAVE_TYPE);
    WMNetBuildStart(1);
#endif
}

void testWMPoll(void)
{
#if DEV_MASTER
    masterPoll();
#else
    slavePoll();
#endif
    WMPoll();
}

