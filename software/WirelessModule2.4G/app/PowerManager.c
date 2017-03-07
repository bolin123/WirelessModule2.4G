#include "PowerManager.h"
#include "WirelessModule.h"
#include "MProto.h"
#include "HalPower.h"

void PMSleep(void)
{
    SysLog("");
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
      && MProtoGotDeviceInfo()
      && NetSendListEmpty())
    {
        PMSleep();
    }
}

