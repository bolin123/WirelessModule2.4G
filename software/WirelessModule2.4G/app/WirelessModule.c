#include "WirelessModule.h"
#include "SysTimer.h"
#include "DevicesManager.h"
#include "HalFlash.h"

#define WM_DEVICES_NUM_MAX 32

typedef enum
{
    WM_MS_MODE_SLAVE,
    WM_MS_MODE_MASTER,
    WM_MS_MODE_NONE = 0xff,
}WMMasterSlaveMode_t;

typedef struct 
{
    bool sleep;
    uint8_t type[NET_DEV_TYPE_LEN];
    uint32_t uid;
}WMSearchDeviceInfo_t;

typedef struct
{
    uint8_t netAddr;
    uint8_t rfChannel;
    uint8_t segAddr[NET_SEG_ADDR_LEN];
}WMNetbuildInfo_t;

/*心跳时间*/
static SysTime_t g_lastHbTime;
static SysTime_t g_hbIntervalTime;

static WMNetbuildInfo_t g_netbuildInfo; //设备自己的组网信息，需要保存至flash
static WMEventReport_cb g_eventHandle = NULL;
static uint8_t g_myDevType[NET_DEV_TYPE_LEN]; //设备自己的设备类型
static bool g_isSleepDevice = false; //是否为休眠设备
static WMNetStatus_t g_netStatus = WM_STATUS_NET_IDLE;
static WMSearchDeviceInfo_t g_searchDevInfo[WM_SEARCH_DEVICE_NUM]; //搜索到的设备信息
static WMMasterSlaveMode_t g_mode;

static bool needHandleTheEvent(NetEventType_t event)
{
    if(g_mode == WM_MS_MODE_MASTER)
    {
        if(event == NET_EVENT_SEARCH \
            || event == NET_EVENT_DEV_ADD \
            || event == NET_EVENT_DEV_DEL \
            || event == NET_EVENT_COORDINATION)
        {
            return false;
        }
    }
    else if(g_mode == WM_MS_MODE_SLAVE)
    {
        if(event == NET_EVENT_DEV_INFO)
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    return true;
}

static void delayReportDeviceInfo(void *args)
{
    NetbuildSearchResponse(g_myDevType, g_isSleepDevice);
}

static void delayStartHeartbeat(void *args)
{
    NetSendHeartbeat(WM_MASTER_NET_ADDR);
    g_lastHbTime = SysTime();
}

static void subDevHeartbeat(void)
{
    if(g_netStatus == WM_STATUS_NET_BUILDED && g_mode == WM_MS_MODE_SLAVE)
    {
        if(SysTimeHasPast(g_lastHbTime, g_hbIntervalTime))
        {
            NetSendHeartbeat(WM_MASTER_NET_ADDR);
            g_lastHbTime = SysTime();
        }
    }
}

static void searchDeviceInfoAdd(uint32_t uid, uint8_t *type, bool sleep)
{
    uint8_t i, id;
    bool ignore = false;

    for(i = WM_SEARCH_DEVICE_NUM; i > 0; i--)
    {
        if(g_searchDevInfo[i - 1].uid == 0)
        {
            id = i - 1;
        }
        if(g_searchDevInfo[i - 1].uid == uid)
        {
            ignore = true;
            break;
        }
    }

    if(!ignore)
    {
        g_searchDevInfo[id].sleep = sleep;
        g_searchDevInfo[id].uid = uid;
        memcpy(g_searchDevInfo[id].type, type, NET_DEV_TYPE_LEN);
    }
}

static void searchDeviceInfoClear(void)
{
    memset(g_searchDevInfo, 0, sizeof(WMSearchDeviceInfo_t) * WM_SEARCH_DEVICE_NUM);
}

/*组网状态设置*/
static void netBuildStatusSet(WMNetStatus_t status)
{
    if(g_netStatus != status)
    {
        if(g_eventHandle != NULL)
        {
            g_eventHandle(WM_EVENT_NET_STATUS_CHANGE, (void *)status);
        }
    }
    
    g_netStatus = status;
}

WMNetStatus_t WMGetNetStatus(void)
{
    return g_netStatus;
}

static void netBuildDone(void *args)
{
    WMEvent_t event = (WMEvent_t)args;
    
    //send search result
    SysLog("");
    netBuildStatusSet(WM_STATUS_NET_BUILDED);
    NetBuildStop(g_netbuildInfo.rfChannel);

    if(g_eventHandle != NULL)
    {
        g_eventHandle(event, NULL);
    }
}

static void updateNetInfoAndSwitchChannel(bool needSave)
{
    NetSetSegaddress(g_netbuildInfo.segAddr); //网段地址为主设备的MAC地址
    NetSetNetaddress(g_netbuildInfo.netAddr); //设置网络地址
    WMNetBuildStart(false); //切换到工作信道

    if(needSave)
    {
        HalFlashErase(SYS_NET_BUILD_INFO_ADDR);
        HalFlashWrite(SYS_NET_BUILD_INFO_ADDR, &g_netbuildInfo, sizeof(WMNetbuildInfo_t));
    }
}

static void dmEventHandle(uint8_t addr, DMEvent_t event)
{
    SysLog("Dev address %d: %s", addr, event == DM_EVENT_ONLINE ? "online" : "offline");
}

static void wmNetlayerEventHandle(NetEventType_t event, uint32_t from, void *args)
{
    if(!needHandleTheEvent(event))
    {
        SysLog("ignore event %d", event);
        return;
    }

    switch(event)
    {
    case NET_EVENT_SEARCH: //主设备发送搜索消息
        {
            uint8_t *type = (uint8_t *)args;
            SysTime_t delayTime;
            SysLog("NET_EVENT_SEARCH: master %c%c%c%c%c%c", type[0], type[1], type[2], type[3], type[4], type[5]);

            SysRandomSeed(SysTime());
            delayTime = SysRandom() % 800; //随机延时 <800 ms，防止与其他从设备冲突
            SysTimerSet(delayReportDeviceInfo, delayTime, 0, (void *)from);
        }
        break;
    case NET_EVENT_DEV_INFO: //从设备上报的设备信息
        {
            NetbuildSubDeviceInfo_t *devInfo = (NetbuildSubDeviceInfo_t *)args;
            SysLog("NET_EVENT_DEV_INFO: subDev type:%c%c%c%c%c%c, sleep:%d, uid:%x", devInfo->type[0], devInfo->type[1], \
                devInfo->type[2], devInfo->type[3], devInfo->type[4], devInfo->type[5], devInfo->sleep, from);
            searchDeviceInfoAdd(from, devInfo->type, devInfo->sleep);
        }
        break;
    case NET_EVENT_DEV_ADD: //主设备添加消息
        {
            uint16_t hbtime;
            NetbuildAddSubInfo_t *netInfo = (NetbuildAddSubInfo_t *)args;
            g_netbuildInfo.netAddr = netInfo->addr;
            g_netbuildInfo.rfChannel = netInfo->rfChannel;
            memcpy(g_netbuildInfo.segAddr, SysUidToMac(from), NET_SEG_ADDR_LEN);

            hbtime = netInfo->hbTime[0];
            hbtime = (hbtime << 8) + netInfo->hbTime[1];

            SysLog("NET_EVENT_DEV_ADD: my address = %d, rfChannel = %d, key = %d, hbtime = %d", \
                    netInfo->addr, netInfo->rfChannel, netInfo->key, hbtime);
            
            DMDeviceSet(NET_MASTER_NET_ADDR, netInfo->key, from, netInfo->masterType, false); //设置主设备信息
            updateNetInfoAndSwitchChannel(true); //设置网络信息，切换通道
            netBuildStatusSet(WM_STATUS_NET_BUILDED);
            SysTimerSet(delayStartHeartbeat, hbtime, 0, NULL);//根据心跳时间开始发送心跳
        }
        break;
    case NET_EVENT_DEV_DEL:
        SysLog("NET_EVENT_DEV_DEL !");
        memset(&g_netbuildInfo, 0xff, sizeof(WMNetbuildInfo_t));
        updateNetInfoAndSwitchChannel(true);
        netBuildStatusSet(WM_STATUS_NET_IDLE);
        break;
    case NET_EVENT_COORDINATION:
        {
            NetCoordinationDev_t *cooDev = (NetCoordinationDev_t *)args;
            SysLog("NET_EVENT_COORDINATION: build=%d, key=%d, netAddr=%d, type=%c%c%c%c%c%c, uid=%d", \
                    cooDev->isBuild, cooDev->key, cooDev->netAddr, cooDev->devType[0], cooDev->devType[1], \
                    cooDev->devType[2], cooDev->devType[3], cooDev->devType[4], cooDev->devType[5], cooDev->uid);
            if(cooDev->isBuild)
            {
                DMDeviceSet(cooDev->netAddr, cooDev->key, cooDev->uid, cooDev->devType, cooDev->sleep);
            }
            else
            {
                DMDeviceDel(DMDeviceAddressFind(cooDev->netAddr));
            }
            // TODO: event launch
        }
        break;
    case NET_EVENT_DEVICE_HEARTBEAT:
        if(DMDeviceAddressFind(from) != NULL)
        {
            SysLog("NET_EVENT_DEVICE_HEARTBEAT subAddr = %d", from);
            DMUpdateHeartbeat((uint8_t)from);
            if(g_mode == WM_MS_MODE_MASTER)
            {
                NetSendHeartbeat(from); //ack heartbeat
            }
        }
        break;
    case NET_EVENT_USER_DATA:
        if(DMDeviceAddressFind(from) != NULL)
        {
            DMUpdateHeartbeat((uint8_t)from);
            SysLog("NET_EVENT_USER_DATA from [%d]", from);
        }
        break;
    default:
        break;
    }
}

void WMNetUserDataSend(uint8_t to, uint8_t *data, uint8_t len)
{
    DMDevicesInfo_t *info = DMDeviceAddressFind(to);

    if(info != NULL)
    {
        NetUserDataSend(to, info->netInfo.sleep, data, len);
    }
}

uint8_t WMQueryDeviceInfo(WMQueryType_t type, uint8_t *contents)
{
    return 0;
}

uint8_t WMGetSearchResult(WMSearchResult_t result[WM_SEARCH_DEVICE_NUM])
{
    uint8_t i, count = 0;

    for(i = 0; i < WM_SEARCH_DEVICE_NUM; i++)
    {
        if(g_searchDevInfo[i].uid != 0)
        {
            result[i].sid = i;
            result[i].sleep = g_searchDevInfo[i].sleep;
            memcpy(result[i].type, g_searchDevInfo[i].type, NET_DEV_TYPE_LEN);
            count++;
        }
    }

    return count;
}

int8_t WMNetBuildDelDevice(uint8_t *addr, uint8_t num)
{
    int8_t ret = 0;
    uint8_t i;
    DMDevicesInfo_t *device;

    if(num > DM_DEVICES_NUM_MAX)
    {
        return -1;
    }

    for(i = 0; i < num; i++)
    {
        device = DMDeviceAddressFind(addr[i]);

        if(device != NULL)
        {
            NetbuildDelDevice(addr[i], device->netInfo.sleep);
            DMDeviceDel(device);
        }
        else
        {
            SysErrLog("device %d not found!", addr[i]);
            ret = -2;
        }
    }
    return ret;
}

int8_t WMNetBuildAddDevice(uint8_t *id, uint8_t num)
{
    uint8_t i;
    uint8_t subAddr, keyVal;
    uint16_t hbTime, switchTime;
    NetbuildAddSubInfo_t subInfo;
    WMSearchDeviceInfo_t *searchInfo;

    if(g_netStatus == WM_STATUS_NET_BUILDING)
    {
        SysErrLog("wait, already building ...");
        return -1;
    }
    
    if(num > WM_SEARCH_DEVICE_NUM || num == 0)
    {
        SysErrLog("num[%d] out of rang!", num);
        return -1;
    }
    
    netBuildStatusSet(WM_STATUS_NET_BUILDING);
    NetBuildStart();
    
    for(i = 0; i < num; i++)
    {
        if(id[i] < WM_SEARCH_DEVICE_NUM)
        {
            searchInfo = &g_searchDevInfo[id[i]];
            subAddr = DMDeviceUidToAddress(searchInfo->uid);
            if(subAddr == DM_DEVICE_INVALID_FLAG)
            {
                SysRandomSeed(SysTime()); //随机数产生 key
                keyVal = (uint8_t)SysRandom();
                subAddr = DMDeviceCreate(searchInfo->sleep, keyVal, searchInfo->uid, searchInfo->type);
            }
            if(subAddr == DM_DEVICE_INVALID_FLAG)
            {
                SysErrLog("device list full!!!");
                return -1;
            }
            //fill subInfo
            subInfo.addr = subAddr;
            subInfo.key = keyVal;
            subInfo.rfChannel = g_netbuildInfo.rfChannel;
            memcpy(subInfo.masterType, g_myDevType, NET_DEV_TYPE_LEN);
            
            //随机分配发送心跳的时间
            switchTime = (num - i) * 200 * 3;
            hbTime = switchTime + SysRandom()%1000;
            subInfo.hbTime[0] = (uint8_t)(hbTime >> 8);
            subInfo.hbTime[1] = (uint8_t)hbTime;
            
            SysLog("Add device UID:%x, addr:%d, key:%d, hbTime:%d", searchInfo->uid,
                    subInfo.addr, subInfo.key, hbTime);
            NetbuildAddDevice(searchInfo->uid, &subInfo);
        }
        else
        {
            SysErrLog("id[%d] out of rang!", id[i]);
            return -1;
        }
    }
    SysTimerSet(netBuildDone, num * 600, 0, (void *)WM_EVENT_ADD_DEV_END);
    return 0;
    
}

static void searchDeviceAgain(void *args)
{
    NetbuildSearchDevice(g_myDevType);
    SysTimerSet(netBuildDone, 1000, 0, (void *)WM_EVENT_SEARCH_END);
}

int8_t WMNetBuildSearch(void)
{
    if(g_netStatus == WM_STATUS_NET_BUILDING)
    {
        SysErrLog("wait, already in search ...");
        return -1;
    }

    if(g_mode == WM_MS_MODE_MASTER)
    {
        netBuildStatusSet(WM_STATUS_NET_BUILDING);
        if(g_netbuildInfo.rfChannel == 0xff) //随机分配通信信道
        {
            SysRandomSeed(SysTime());
            g_netbuildInfo.rfChannel = NET_WORK_START_CH + (SysRandom() % 30);
        }
        searchDeviceInfoClear();
        NetBuildStart();
        NetbuildSearchDevice(g_myDevType);
        SysTimerSet(searchDeviceAgain, 1000, 0, NULL);
        return 0;
    }
    return -1;
}

void WMNetBuildStart(bool start)
{
    if(g_mode == WM_MS_MODE_SLAVE)
    {
        if(start)
        {
            netBuildStatusSet(WM_STATUS_NET_BUILDING);
            NetBuildStart();
        }
        else
        {
            NetBuildStop(g_netbuildInfo.rfChannel);
        }
    }
}

int8_t WMDeviceCoordination(bool isBuild, uint8_t addr1, uint8_t addr2)
{
    NetCoordinationDev_t cooDev;
    DMDevicesInfo_t *device;
    
    if(g_mode == WM_MS_MODE_MASTER)
    {
        device = DMDeviceAddressFind(addr1);
        if(device != NULL)
        {
            cooDev.isBuild = isBuild;
            cooDev.key = device->netInfo.key;
            cooDev.netAddr = addr1;
            cooDev.sleep = device->netInfo.sleep;
            cooDev.uid = device->netInfo.uid;
            memcpy(cooDev.devType, device->netInfo.devType, NET_DEV_TYPE_LEN);
            NetCoordinationOperate(addr2, &cooDev); //将addr1的设备信息发给add2设备
        }
        
        device = DMDeviceAddressFind(addr2);
        if(device != NULL)
        {
            cooDev.isBuild = isBuild;
            cooDev.key = device->netInfo.key;
            cooDev.netAddr = addr2;
            cooDev.sleep = device->netInfo.sleep;
            cooDev.uid = device->netInfo.uid;
            memcpy(cooDev.devType, device->netInfo.devType, NET_DEV_TYPE_LEN);
            NetCoordinationOperate(addr1, &cooDev); //将addr2的设备信息发给add1设备
        }
        
        return 0;
    }
    return -1;
}

void WMSetMasterSlaveMode(bool isMaster)
{
    bool needSave = false;
    SysLog("isMaster = %d", isMaster);
    //TODO different?
    if(isMaster)
    {
        g_mode = WM_MS_MODE_MASTER;
        if(g_netbuildInfo.netAddr != NET_MASTER_NET_ADDR)
        {
            g_netbuildInfo.netAddr = NET_MASTER_NET_ADDR;
            //随机选择通信通道
            SysRandomSeed(SysTime() + NetGetMacAddr(NULL)[3]);
            g_netbuildInfo.rfChannel = NET_WORK_START_CH + (SysRandom() % 30);
            
            NetGetMacAddr(g_netbuildInfo.segAddr);
            needSave = true;
        }
        
        netBuildStatusSet(WM_STATUS_NET_BUILDED);
    }
    else
    {
        g_mode = WM_MS_MODE_SLAVE;
        if(g_netbuildInfo.netAddr == NET_BROADCAST_NET_ADDR)
        {
            netBuildStatusSet(WM_STATUS_NET_IDLE);
        }
        else
        {
            netBuildStatusSet(WM_STATUS_NET_BUILDED);
        }
    }
    
    updateNetInfoAndSwitchChannel(needSave);
}

/*设置休眠模式*/
void WMSetSleepMode(bool isSleep)
{
    SysLog("sleep = %d", isSleep);
    g_isSleepDevice = isSleep;
    g_hbIntervalTime = isSleep ? NET_SLEEP_DEVICE_HBTIME : NET_NORMAL_DEVICE_HBTIME;
    NetSetSleepMode(isSleep);
}

/*设置设备类型*/
void WMSetModuleType(const uint8_t *type)
{
    SysLog("%c%c%c%c%c%c", type[0], type[1], type[2], type[3], type[4], type[5]);
    memcpy(g_myDevType, type, NET_DEV_TYPE_LEN);
}

void WMEventRegister(WMEventReport_cb eventHandle)
{
    g_eventHandle = eventHandle;
}

void WMInitialize(void)
{
    uint8_t mac[NET_MAC_ADDR_LEN] = {0x22, 0x02, 0x03, 0x13};
    NetLayerInit();
    NetLayerEventRegister(wmNetlayerEventHandle);
#if 0
    HalFlashErase(SYS_DEVICE_MAC_ADDR);
    HalFlashWrite(SYS_DEVICE_MAC_ADDR, mac, NET_MAC_ADDR_LEN);
#endif    
    HalFlashRead(SYS_DEVICE_MAC_ADDR, mac, NET_MAC_ADDR_LEN);
    NetSetMacAddr(mac);

    DMInitialize();
    DMEventRegister(dmEventHandle);

#if 0
    WMNetbuildInfo_t info;
    //uint8_t seg[4] = {0x22, 0x02, 0x03, 0x11};
    //uint8_t mac[4] = {0x22, 0x02, 0x03, 0x22};

    info.netAddr = 0;
    info.rfChannel = 100;
    memcpy(info.segAddr, mac, 4);
    //memcpy(info.mac, mac, 4);
    HalFlashErase(SYS_NET_BUILD_INFO_ADDR);
    HalFlashWrite(SYS_NET_BUILD_INFO_ADDR, &info, sizeof(WMNetbuildInfo_t));
#endif
    memset(&g_netbuildInfo, 0xff, sizeof(WMNetbuildInfo_t));
    HalFlashRead(SYS_NET_BUILD_INFO_ADDR, &g_netbuildInfo, sizeof(WMNetbuildInfo_t));

    if(g_netbuildInfo.netAddr != 0xff)
    {
        netBuildStatusSet(WM_STATUS_NET_BUILDED);
        updateNetInfoAndSwitchChannel(false);
    }
    
}

void WMPoll(void)
{
    NetLayerPoll();
    //if !building
    subDevHeartbeat();
    DMPoll();
}

