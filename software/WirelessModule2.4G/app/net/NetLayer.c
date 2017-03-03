#include "NetLayer.h"
#include "Sys.h"
#include "SysTimer.h"
#include "DevicesManager.h"
#include "VTList.h"

#define NET_SLEEP_CACHE_MAX_NUM 10
#define NET_CRYPTION_OFFSET 0x27 //加解密的偏移量

typedef enum
{
    NET_PROTO_TYPE_SEARCH       = 0x01,
    NET_PROTO_TYPE_REPORT_INFO  = 0x02,
    NET_PROTO_TYPE_ADD_DEVICE   = 0x03,
    NET_PROTO_TYPE_DEL_DEVICE   = 0x04,
    NET_PROTO_TYPE_COORDINATION = 0x05,
    NET_PROTO_TYPE_USER_DATA    = 0x06,
    NET_PROTO_TYPE_ACK          = 0x07,
}NetProtocolType_t;

typedef struct SleepDevDataCache_st
{
    uint8_t to;
    uint8_t dlen;
    uint8_t *data;
    SysTime_t sendTime;
    VTLIST_ENTRY(struct SleepDevDataCache_st);
}SleepDevDataCache_t;

static const uint8_t g_broadMac[PHY_MAC_LEN] = { 0xff, 0xff, 0xff, 0xff };
//static NetMode_t g_moduleMode = NET_MODE_NONE;
//static uint8_t g_devicetype[net_dev_type_len];
static bool g_netBuildStart = false;
static bool g_needSleep = false;
static NetEventHandle_cb g_netEventHandle = NULL;
static SleepDevDataCache_t g_sleepDevDataCache;
static uint8_t g_sleepCacheCount = 0;

static int8_t netEnDecrypt(uint8_t addr, uint8_t *input, uint8_t len)
{
    DMDevicesInfo_t *info = DMDeviceAddressFind(addr);

    if(info == NULL)
    {
        SysErrLog("key not found!!!");
        return -1;
    }
    
#if 0
    input = input;
#else
    uint8_t i;
    uint8_t netKey = info->netInfo.key;// ~(info->netInfo.key + NET_CRYPTION_OFFSET);
    uint8_t value;
    
    for(i = 0; i < len; i++)
    {
        value = input[i] ^ netKey;
        input[i] = value;
    }
#endif
    return 0;
}

static void netThrowEvent(NetEventType_t event, uint32_t from, void *args)
{
    if(g_netEventHandle)
    {
        g_netEventHandle(event, from, (void *)args);
    }
}

static void netSleepDevDataSave(uint8_t to, const uint8_t *data, uint8_t dlen)
{
    if(g_sleepCacheCount >= NET_SLEEP_CACHE_MAX_NUM)
    {
        SysLog("sleep cache full!");
        return;
    }
    
    SleepDevDataCache_t *cache = (SleepDevDataCache_t *)malloc(sizeof(SleepDevDataCache_t));

    cache->to = to;
    cache->dlen = dlen;
    cache->data = (uint8_t *)malloc(dlen);
    //encrypt...
    memcpy(cache->data, data, dlen);
    cache->sendTime = SysTime();

    VTListAdd(&g_sleepDevDataCache, cache);
    g_sleepCacheCount++;
}

static void netSleepDevDataHandle(void)
{
    SleepDevDataCache_t *dataCache = VTListFirst(&g_sleepDevDataCache);

    if(dataCache != NULL)
    {
        //超时未收到休眠设备的数据，导致缓存数据未发送，清除消息
        if(SysTimeHasPast(dataCache->sendTime, NET_SLEEP_DEVICE_HBTIME + 5000)) 
        {
            VTListDel(dataCache);
            free(dataCache->data);
            free(dataCache);
            g_sleepCacheCount--;
        }
    }
}

static void findCacheAndSend(uint8_t addr)
{
    SleepDevDataCache_t *dataCache = NULL;

    VTListForeach(&g_sleepDevDataCache, dataCache)
    {
        if(dataCache->to == addr)
        {
            PHYOperatePacketSend(addr, g_needSleep, dataCache->data, dataCache->dlen, false);
            VTListDel(dataCache);
            free(dataCache->data);
            free(dataCache);
            dataCache = NULL;
            g_sleepCacheCount--;
        }
    }
}

static void netOperateFrameHandle(uint8_t srcAddr, bool isSleep, bool isBroadcast, uint8_t *data, uint8_t len)
{
    uint8_t i = 0;
    uint8_t *optData = &data[1];

    if(!isBroadcast)
    {
        if(netEnDecrypt(srcAddr, data, len) < 0)
        {
            return;
        }
    }
    
    if(isSleep)
    {
        findCacheAndSend(srcAddr);
    }
    
    switch(data[0]) //type
    {
    case NET_PROTO_TYPE_DEL_DEVICE:
        netThrowEvent(NET_EVENT_DEV_DEL, srcAddr, NULL);
        break;
    case NET_PROTO_TYPE_COORDINATION:
        {
            NetCoordinationDev_t ordinationDev;
            ordinationDev.isBuild = optData[i++];
            ordinationDev.netAddr = optData[i++];
            ordinationDev.uid = SysMacToUid(&optData[i]);
            i += PHY_MAC_LEN;
            memcpy(ordinationDev.devType, &optData[i], NET_DEV_TYPE_LEN);
            i += NET_DEV_TYPE_LEN;
            ordinationDev.sleep = optData[i++];
            ordinationDev.key = optData[i++];
            netThrowEvent(NET_EVENT_COORDINATION, srcAddr, &ordinationDev);
        }
        break;
#if 0
    case NET_PROTO_TYPE_HEARTBEAT:
        netThrowEvent(NET_EVENT_DEVICE_HEARTBEAT, srcAddr, NULL);
        break;
#endif
    case NET_PROTO_TYPE_USER_DATA:
        {
            NetUserData_t userData;
            userData.isBroadcast = isBroadcast;
            userData.dlen = len - 1;
            userData.data = optData;
            netThrowEvent(NET_EVENT_USER_DATA, srcAddr, &userData);
        }
        break;
    default:
        break;
    }
}

static void netHeartbeatHandle(uint8_t addr, bool sleep)
{
    if(sleep)
    {
        findCacheAndSend(addr);
    }

    netThrowEvent(NET_EVENT_DEVICE_HEARTBEAT, addr, NULL);
}

/*
void NetbuildSubReportDevInfo(uint32_t masterUID, NetbuildSubReportInfo_t *devInfo)
{
    uint8_t data[64];
    uint8_t i = 0;

    data[i++] = NET_PROTO_TYPE_REPORT_INFO;
    memcpy(&data[i], devInfo->devType, NET_DEV_TYPE_LEN);
    i += NET_DEV_TYPE_LEN;
    data[i++] = devInfo->sleep;

    PHYEstnetPacketSend(SysUidToMac(masterUID), data, i, false);
}
*/

void NetSendHeartbeat(uint8_t addr)
{
#if 0
    uint8_t hb[1];

    hb[0] = NET_PROTO_TYPE_HEARTBEAT;
    //encrypt
    PHYOperatePacketSend(addr, g_needSleep, hb, sizeof(hb), true);
#endif
    PHYHeartbeatSend(addr, g_needSleep);
}

static void netBuildFrameHandle(uint8_t srcMac[PHY_MAC_LEN], bool isBroadcast, uint8_t *data, uint8_t len)
{
//    uint8_t i = 0;
    uint8_t type = data[0];
    uint8_t *content = &data[1];
    uint32_t uid = SysMacToUid(srcMac);

    if(!g_netBuildStart)
    {
        return;
    }

    switch(type)
    {
        case NET_PROTO_TYPE_SEARCH:
            netThrowEvent(NET_EVENT_SEARCH, uid, (void *)content);
            break;
        case NET_PROTO_TYPE_REPORT_INFO:
            netThrowEvent(NET_EVENT_DEV_INFO, uid, (void *)content);
            break;
        case NET_PROTO_TYPE_ADD_DEVICE:
            netThrowEvent(NET_EVENT_DEV_ADD, uid, (void *)content);
            break;
        default:
            break;
    }
    
}

void NetBuildStart(void)
{
    SysLog("");
    g_netBuildStart = true;
    PHYSwitchChannel(NET_BUILD_CH_NUM);
}

void NetBuildStop(uint8_t workCh)
{
    SysLog("work ch:%d", workCh);
    g_netBuildStart = false;
    PHYSwitchChannel(workCh);
}

void NetUserDataSend(uint8_t to, bool isSleep, uint8_t *data, uint8_t len)
{
    uint8_t usrData[255];
    
    usrData[0] = NET_PROTO_TYPE_USER_DATA;
    memcpy(&usrData[1], data, len);

    if(to != NET_BROADCAST_NET_ADDR)
    {
        netEnDecrypt(to, usrData, len + 1);
    }
    
    if(isSleep) //发往的设备休眠属性
    {
        netSleepDevDataSave(to, usrData, len + 1); //若为休眠设备，先缓存，等设备唤醒时再发送
    }
    else
    {
        PHYOperatePacketSend(to, g_needSleep, usrData, len + 1, false);
    }
}

void NetCoordinationOperate(uint8_t to, bool sleep, NetCoordinationDev_t *cooDev)
{
    uint8_t data[32], i = 0;

    data[i++] = NET_PROTO_TYPE_COORDINATION;
    data[i++] = cooDev->isBuild;
    data[i++] = cooDev->netAddr;
    memcpy(&data[i], SysUidToMac(cooDev->uid), PHY_MAC_LEN);
    i += PHY_MAC_LEN;
    memcpy(&data[i], cooDev->devType, NET_DEV_TYPE_LEN);
    i += NET_DEV_TYPE_LEN;
    data[i++] = cooDev->sleep;
    data[i++] = cooDev->key;

    netEnDecrypt(to, data, i);
    
    if(sleep)
    {
        netSleepDevDataSave(to, data, i);
    }
    else
    {
        PHYOperatePacketSend(to, g_needSleep, data, i, false);
    }
}

void NetSetSleepMode(bool needSleep)
{
    SysLog("%d", needSleep);
    g_needSleep = needSleep;
}

void NetSetNetaddress(uint8_t addr)
{
    PHYSetNetAddr(addr);
}

void NetSetSegaddress(const uint8_t *segAddr)
{
    PHYSetSegAddr(segAddr);
}

void NetSetMacAddr(const uint8_t *mac)
{
    SysLog("%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3]);
    PHYSetMac(mac);
}

uint8_t *NetGetMacAddr(uint8_t *mac)
{
    return PHYGetMac(mac);
}

/***************************Build network**********************************/
//master module interface
void NetbuildSearchDevice(const uint8_t myType[NET_DEV_TYPE_LEN])
{
    SysLog("");
    uint8_t data[NET_DEV_TYPE_LEN + 1];
    data[0] = NET_PROTO_TYPE_SEARCH;
    memcpy(&data[1], myType, NET_DEV_TYPE_LEN);
    PHYEstnetPacketSend(g_broadMac, (const uint8_t *)data, sizeof(data), false);
}

void NetbuildAddDevice(uint32_t uid, NetbuildAddSubInfo_t *subInfo)
{
    uint8_t data[sizeof(NetbuildAddSubInfo_t) + 1];

    data[0] = NET_PROTO_TYPE_ADD_DEVICE;
    memcpy(&data[1], subInfo, sizeof(NetbuildAddSubInfo_t));
    PHYEstnetPacketSend(SysUidToMac(uid), data, sizeof(NetbuildAddSubInfo_t) + 1, false);
}

void NetbuildDelDevice(uint8_t addr, bool isSleep)
{
    uint8_t data[1];
    
    netEnDecrypt(addr, data, sizeof(data));
    data[0] = NET_PROTO_TYPE_DEL_DEVICE;
    if(isSleep) //休眠设备先缓存
    {
        netSleepDevDataSave(addr, data, sizeof(data));
    }
    else
    {
        PHYOperatePacketSend(addr, g_needSleep, data, sizeof(data), false);
    }
}

//slave module interface
void NetbuildSearchResponse(const uint8_t *devType, bool isSleep)
{
    uint8_t data[NET_DEV_TYPE_LEN + 2];
    //uint8_t mac[PHY_MAC_LEN];

    //memcpy(mac, SysUidToMac(uid), PHY_MAC_LEN);
    data[0] = NET_PROTO_TYPE_REPORT_INFO;
    memcpy(&data[1], devType, NET_DEV_TYPE_LEN);
    data[NET_DEV_TYPE_LEN + 1] = isSleep;
    PHYEstnetPacketSend(g_broadMac, data, sizeof(data), true);
}

void NetLayerSetSleepMode(bool sleep)
{
    if(sleep)
    {
    }
    else
    {
    }
}

void NetLayerEventRegister(NetEventHandle_cb eventCb)
{
    g_netEventHandle = eventCb;
}

void NetLayerInit(void)
{
    VTListInit(&g_sleepDevDataCache);
    PHYInitialize();
    PHYPacketHandleCallbackRegiste(netOperateFrameHandle, netBuildFrameHandle, netHeartbeatHandle);
}

void NetLayerPoll(void)
{
    PHYPoll();
    netSleepDevDataHandle();
}
