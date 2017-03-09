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

/*�����豸����*/
void PMPoll(void)
{
    /*�ܽ���Ϊ���ߣ�����������״̬���յ��豸��Ϣ�����Ͷ���Ϊ��*/
    if(HalPowerNeedSleep()
      && WMGetNetStatus() != WM_STATUS_NET_BUILDING
      && SysGotDeviceInfo()
      && SysDataSendListEmpty())
    {
        PMSleep();
    }
}

