#include "testWM.h"
#include "HalGPIO.h"
#include "WirelessModule.h"

#define DEV_MASTER  0

#define SLAVE_TYPE "SLV203"
#define MASTER_TYPE "MST001"
#define STATUS_LED_PIN 0x11  //PB1

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

static void slavePoll(void)
{
    static SysTime_t lastTime;
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
}

static void testEventHandle(WMEvent_t event, void *args)
{
    uint8_t i, num;
    WMSearchResult_t result[WM_SEARCH_DEVICE_NUM];
    uint8_t devNum[WM_SEARCH_DEVICE_NUM];

    SysLog("event %d", event);
    
    if(WM_EVENT_SEARCH_END == event)
    {
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
    }
    else if(WM_EVENT_NET_STATUS_CHANGE == event)
    {
        WMNetStatus_t  netStatus = (WMNetStatus_t)args;
        
        if(WM_STATUS_NET_IDLE == netStatus)
        {
            HalGPIOSet(STATUS_LED_PIN, HAL_GPIO_LEVEL_HIGH);
        }
        else if(WM_STATUS_NET_BUILDING == netStatus)
        {
        }
        else //WM_STATUS_NET_BUILDED
        {
            HalGPIOSet(STATUS_LED_PIN, HAL_GPIO_LEVEL_LOW);
        }
    }
    
}

void testWMInit(void)
{
    HalGPIOInit(STATUS_LED_PIN, HAL_GPIO_DIR_OUT);
    HalGPIOSet(STATUS_LED_PIN, HAL_GPIO_LEVEL_HIGH);
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
    WMEventRegister(testEventHandle);
}

void testWMPoll(void)
{
#if DEV_MASTER
    masterPoll();
#else
    slavePoll();
#endif
}

