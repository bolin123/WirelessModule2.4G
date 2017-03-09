#include "PowerManager.h"
#include "WirelessModule.h"
#include "HalPower.h"

void PMSetSleepStatus(bool sleep)
{
    HalSetSleepPinStatus(sleep);
}

void PMSleep(void)
{
    SysLog("");
    PMSetSleepStatus(true);
    NetLayerSleep(true); //RF sleep
    HalPowerSleep();     //MCU sleep
}

void PMWakeUp(void)
{
    HalPowerWakeup();      //MCU wakeup
    NetLayerSleep(false);  //RF wakeup
    SysLog("");
}

void PMInitialize(void)
{
    HalPowerInitialize();
}

/*休眠设备调用*/
void PMPoll(void)
{
    /*管脚置为休眠，不处在配网状态，收到设备信息，发送队列为空*/
    if(HalPowerNeedSleep()
      && WMGetNetStatus() != WM_STATUS_NET_BUILDING
      && SysGotDeviceInfo()
      && SysDataSendListEmpty())
    {
        PMSleep();
    }
}

