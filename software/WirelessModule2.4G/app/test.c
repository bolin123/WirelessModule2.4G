#include "NetLayer.h"
#include "SysTimer.h"

static void testBuildNetAdd(void *args)
{
    uint8_t addDev[18] = { 0xA5, 0x01, 0x01, 0xA4, 0x3F, 0x33, 0x01, 0xA4, 0x3F, 0x34, 0x03, 0x06, 0x03, 0x02, 0x70, 0x34, 0x00, 0x80 };
    
    SysLog("");
    PHYPacketRecvHandle(addDev, sizeof(addDev));

}

static void testBuildNetSearch(void *args)
{
    uint8_t search[19] = {0xA5, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0xA4, 0x3F, 0x34, 0x01, 0x07, 0x01, 0x4d, 0x4f, 0x44, 0x31, 0x32, 0x33};
    
    SysLog("");
    PHYPacketRecvHandle(search, sizeof(search));

    SysTimerSet(testBuildNetAdd, 2000, 0, NULL);
}

static void netEventHandle(NetEventType_t event, uint32_t from, void *args)
{
    SysLog("event:%d, uid:%d", event, from);
    if(event == NET_EVENT_SEARCH)
    {
        uint8_t *type = (uint8_t *)args;
        SysLog("[%c%c%c%c%c%c] search...", type[0], type[1], type[2], type[3], type[4], type[5]);
    }
    else if(event == NET_EVENT_DEV_ADD)
    {
        NetbuildAddSubInfo_t *netInfo = (NetbuildAddSubInfo_t *)args;
        SysLog("Net Info: add:%02x, rfCh:%d, key:%02x", netInfo->addr, netInfo->rfChannel, netInfo->key);
        NetBuildStop();

        HalPrintf("Search device:\n");
        NetbuildSearchDevice();

        HalPrintf("Add device:\n");
        NetbuildAddSubInfo_t subInfo;
        subInfo.addr = 0x01;
        subInfo.key = 0x99;
        subInfo.rfChannel = 0x70;
        subInfo.hbTime[0] = 0x00;
        subInfo.hbTime[1] = 0x7f;
        NetbuildAddDevice(27541300, &subInfo);

        HalPrintf("Del device:\n");
        NetbuildDelDevice(0x01);

        HalPrintf("Coordination device:\n");
        NetCoordinationDev_t coDev;
        coDev.uid = 27541300;
        coDev.isBuild = 1;
        coDev.key = 0x99;
        coDev.netAddr = 0x01;
        memcpy(coDev.devType, "MOD123", NET_DEV_TYPE_LEN);
        NetCoordinationOperate(0x01, false, &coDev);

        uint8_t usrData[10] = {1,2,3,4,5,6,7,8,9,0};
        NetUserDataSend(0x01, usrData, sizeof(usrData));
    }
}

void testInit(void)
{
    uint8_t mac[4] = { 0x01, 0xA4, 0x3F, 0x33 };
    uint8_t type[6] = { 0x53, 0x55, 0x42, 0x31, 0x32, 0x33 };
    NetSetMacAddr(mac);
    NetSetModuleType(type);
    NetSetSleepMode(0);
    NetSetModuleMode(NET_MODE_SLAVE);
    NetLayerEventRegister(netEventHandle);
    NetBuildStart();
    SysTimerSet(testBuildNetSearch, 2000, 0, NULL);
}

void testPoll(void)
{
}
