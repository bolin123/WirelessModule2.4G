#include "WirelessModule.h"
#include "SysTimer.h"
#include "DevicesManager.h"
#include "PowerManager.h"
#include "HalFlash.h"

#define WM_DEVICES_NUM_MAX 32

typedef enum
{
    WM_MS_MODE_SLAVE = 0,
    WM_MS_MODE_MASTER = 1,
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
static SysTime_t g_lastHbTime = 0;
static SysTime_t g_hbIntervalTime = NET_NORMAL_DEVICE_HBTIME;

static WMNetbuildInfo_t g_netbuildInfo; //设备自己的组网信息，需要保存至flash
static WMEventReport_cb g_eventHandle = NULL;
static uint8_t g_myDevType[NET_DEV_TYPE_LEN]; //设备自己的设备类型
static bool g_isSleepDevice = false; //是否为休眠设备
static WMNetStatus_t g_netStatus = WM_STATUS_NET_IDLE;
static WMSearchDeviceInfo_t g_searchDevInfo[WM_SEARCH_DEVICE_NUM]; //搜索到的设备信息
static WMMasterSlaveMode_t g_mode = WM_MS_MODE_NONE;
static bool g_onlineStatus = false;

static uint32_t getRandomValue(uint32_t seed)
{
    SysRandomSeed(seed);
    return SysRandom();
}

static bool needHandleTheEvent(NetEventType_t event)
{
    if(!SysGotDeviceInfo()) //获取到设备信息后才进行消息处理
    {
        return false;
    }
    
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

/*从设备网络心跳*/
static void subDevHeartbeat(void)
{
    if(g_netStatus == WM_STATUS_NET_BUILDED 
        && g_mode == WM_MS_MODE_SLAVE
        && g_isSleepDevice == false) //休眠设备被唤醒时发送一次心跳，不再定时发送
    {
        if(g_lastHbTime == 0 || SysTimeHasPast(g_lastHbTime, g_hbIntervalTime))
        {
            SysLog("");
            NetSendHeartbeat(WM_MASTER_NET_ADDR);
            g_lastHbTime = SysTime();
            /*下次心跳加1s以内的随机数，避免冲突*/
            g_hbIntervalTime = NET_NORMAL_DEVICE_HBTIME + getRandomValue(SysTime()) % 1000;
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
    uint8_t blink = 1;
    
    if(g_netStatus != status)
    {
        /*设置状态LED闪烁的次数*/
        switch(status)
        {
            case WM_STATUS_NET_IDLE:
                blink = 3;
                break;
            case WM_STATUS_NET_BUILDING:
                blink = 2;
                break;
            case WM_STATUS_NET_BUILDED:
                blink = 1;
                break;
            default:
                break;
        }
        HalStatusLedSet(blink); 
        
        if(g_eventHandle != NULL)
        {
            g_eventHandle(WM_EVENT_NET_STATUS_CHANGE, (void *)status);
        }
    }
    
    g_netStatus = status;
}

/*网络状态
* 0=空闲
* 1=正在组网
* 2=已组网
*/
WMNetStatus_t WMGetNetStatus(void)
{
    return g_netStatus;
}

/*本设备是否在线*/
bool WMIsDeviceOnline(void)
{
    if(g_mode == WM_MS_MODE_MASTER)
    {
        return true; //主设备始终在线
    }
    else
    {
        return g_onlineStatus;
    }
}

static void netBuildDone(void *args)
{
    WMEvent_t event = (WMEvent_t)((uint32_t)args);
    
    SysLog("");
    netBuildStatusSet(WM_STATUS_NET_BUILDED);
    NetBuildStop(g_netbuildInfo.rfChannel);

    if(g_eventHandle != NULL)
    {
        g_eventHandle(event, NULL);
    }
}

/*更新本设备的配网信息
* @needSave:是否存储
*/
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

/*设备上下线处理函数*/
static void dmEventHandle(uint8_t addr, DMEvent_t event)
{
    SysLog("Dev address %d: %s", addr, event == DM_EVENT_ONLINE ? "online" : "offline");

    if(g_mode == WM_MS_MODE_SLAVE)
    {
        if(addr == NET_MASTER_NET_ADDR) //从设备将修改自身的状态
        {
            g_onlineStatus = (event == DM_EVENT_ONLINE ? true : false);
        }
    }
    else
    {
        if(event == DM_EVENT_ONLINE)
        {
            g_eventHandle(WM_EVENT_ONLINE, (void *)addr);
        }
        else
        {
            g_eventHandle(WM_EVENT_OFFLINE, (void *)addr);
        }
    }
}

/*搜索回调函数
* @uid:设备的唯一码(由mac转换)
* @devInfo:从设备信息
* 只有搜索到从设备才回调该函数
*/
static void wmSearchResultHandle(uint32_t uid, NetbuildSubDeviceInfo_t *devInfo)
{
    SysLog("subDev type:%c%c%c%c%c%c, sleep:%d, uid:%x", devInfo->type[0], devInfo->type[1], \
        devInfo->type[2], devInfo->type[3], devInfo->type[4], devInfo->type[5], devInfo->sleep, uid);
    searchDeviceInfoAdd(uid, devInfo->type, devInfo->sleep);
}

/*网络事件处理函数*/
static void wmEventHandle(NetEventType_t event, uint32_t from, void *args)
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
            SysLog("SEARCH: master %c%c%c%c%c%c", type[0], type[1], type[2], type[3], type[4], type[5]);

            delayTime = getRandomValue(SysTime()) % 800; //随机延时 <800 ms，防止与其他从设备冲突
            SysTimerSet(delayReportDeviceInfo, delayTime, 0, (void *)from);
        }
        break;
    #if 0
    case NET_EVENT_DEV_INFO: //从设备上报的设备信息
        {
            NetbuildSubDeviceInfo_t *devInfo = (NetbuildSubDeviceInfo_t *)args;
            SysLog("NET_EVENT_DEV_INFO: subDev type:%c%c%c%c%c%c, sleep:%d, uid:%x", devInfo->type[0], devInfo->type[1], \
                devInfo->type[2], devInfo->type[3], devInfo->type[4], devInfo->type[5], devInfo->sleep, from);
            searchDeviceInfoAdd(from, devInfo->type, devInfo->sleep);
        }
        break;
    #endif
    case NET_EVENT_DEV_ADD: //主设备添加消息
        {
            uint16_t hbtime;
            NetbuildAddSubInfo_t *netInfo = (NetbuildAddSubInfo_t *)args;
            g_netbuildInfo.netAddr = netInfo->addr;
            g_netbuildInfo.rfChannel = netInfo->rfChannel;
            memcpy(g_netbuildInfo.segAddr, SysUidToMac(from), NET_SEG_ADDR_LEN);

            hbtime = netInfo->hbTime[0];
            hbtime = (hbtime << 8) + netInfo->hbTime[1];

            SysLog("DEV_ADD: my address = %d, rfChannel = %d, key = %d, hbtime = %d", \
                    netInfo->addr, netInfo->rfChannel, netInfo->key, hbtime);
            
            DMMasterDeviceSet(netInfo->key, from, netInfo->masterType, g_hbIntervalTime); //设置主设备信息
            updateNetInfoAndSwitchChannel(true); //设置网络信息，切换通道
            netBuildStatusSet(WM_STATUS_NET_BUILDED);
            SysTimerSet(delayStartHeartbeat, hbtime, 0, NULL);//根据心跳时间开始发送心跳
        }
        break;
    case NET_EVENT_DEV_DEL:
        SysLog("DEV_DEL !");
        memset(&g_netbuildInfo, 0xff, sizeof(WMNetbuildInfo_t));
        updateNetInfoAndSwitchChannel(true);
        if(g_eventHandle != NULL)
        {
            g_eventHandle(WM_EVENT_DELETED, NULL);
        }
        netBuildStatusSet(WM_STATUS_NET_IDLE);
        break;
    case NET_EVENT_COORDINATION:
        {
            NetCoordinationDev_t *cooDev = (NetCoordinationDev_t *)args;
            WMCoordinationData_t cooData;
            SysLog("COORDINATION: build=%d, key=%d, netAddr=%d, type=%c%c%c%c%c%c, uid=%d, sleep=%d", \
                    cooDev->isBuild, cooDev->key, cooDev->netAddr, cooDev->devType[0], cooDev->devType[1], \
                    cooDev->devType[2], cooDev->devType[3], cooDev->devType[4], cooDev->devType[5], \
                    cooDev->uid, cooDev->sleep);

            if(cooDev->isBuild)
            {
                DMDeviceSet(cooDev->netAddr, cooDev->key, cooDev->uid, cooDev->devType, cooDev->sleep);
            }
            else
            {
                DMDeviceDel(DMDeviceAddressFind(cooDev->netAddr));
            }
            
            if(g_eventHandle != NULL)
            {
                cooData.build   = cooDev->isBuild;
                cooData.address = cooDev->netAddr;
                cooData.sleep   = cooDev->sleep;
                memcpy(cooData.type, cooDev->devType, NET_DEV_TYPE_LEN);
                g_eventHandle(WM_EVENT_COORDINATION, (void *)&cooData);
            }
        }
        break;
    case NET_EVENT_DEVICE_HEARTBEAT:
        if(DMDeviceAddressFind(from) != NULL)
        {
            SysLog("HEARTBEAT, addr = %d", from);
            DMUpdateOnlineTime((uint8_t)from);
            if(g_mode == WM_MS_MODE_MASTER)
            {
                NetSendHeartbeat(from); //ack heartbeat
            }
        }
        break;
    case NET_EVENT_USER_DATA:
        {
            NetUserData_t *netData = (NetUserData_t *)args;
            WMUserData_t userData;
            if(from == NET_BROADCAST_NET_ADDR || DMDeviceAddressFind(from) != NULL)
            {
                DMUpdateOnlineTime((uint8_t)from);
                userData.from = (uint8_t)from;
                userData.isBroadcast = netData->isBroadcast;
                userData.dlen = netData->dlen;
                userData.data = netData->data;
                
                SysLog("USER_DATA from [%d], length %d", from, netData->dlen);
                if(g_eventHandle != NULL)
                {
                    g_eventHandle(WM_EVENT_USER_DATA, (void *)&userData);
                }
            }
        }
        break;
    default:
        break;
    }
}

/*发送用户数据
@to:目的地址，FF为广播地址
*/
void WMNetUserDataSend(uint8_t to, uint8_t *data, uint8_t len)
{
    DMDevicesInfo_t *info = NULL;
    SysLog("to:%d, len:%d", to, len);

    if(to == NET_BROADCAST_NET_ADDR)
    {
        NetUserDataSend(to, false, data, len);
    }
    else
    {
        info = DMDeviceAddressFind(to);
        if(info != NULL)
        {
            NetUserDataSend(to, info->netInfo.sleep, data, len);
        }
    }
    
}

/*查询设备信息
* @type:信息类型(关联、在线)
*/
uint8_t WMQueryDeviceInfo(WMQueryType_t type, uint8_t *contents)
{
    SysLog("type=%d", type);
    if(type == WMQUERY_TYPE_ONLINE)
    {
        return DMGetOnlineDeviceType(contents);
    }
    else
    {
        return DMGetRelatedDeviceType(contents);
    }
}

/*取搜索到的从设备信息*/
uint8_t WMGetSearchResult(uint8_t *result)
{
    uint8_t i, count = 0;

    SysLog("");
    for(i = 0; i < WM_SEARCH_DEVICE_NUM; i++)
    {
        if(g_searchDevInfo[i].uid != 0)
        {
            result[i * sizeof(WMSearchResult_t)] = i;
            memcpy(&result[i * sizeof(WMSearchResult_t) + 1], g_searchDevInfo[i].type, NET_DEV_TYPE_LEN);
            result[i * sizeof(WMSearchResult_t) + NET_DEV_TYPE_LEN + 1] = g_searchDevInfo[i].sleep;
            count++;
        }
    }

    return (count * sizeof(WMSearchResult_t));
}

/*从网络中删除从设备*/
int8_t WMNetBuildDelDevice(uint8_t *addr, uint8_t num)
{
    int8_t ret = 0;
    uint8_t i;
    DMDevicesInfo_t *device;

    SysLog("");

    if(g_mode != WM_MS_MODE_MASTER)
    {
        SysErrLog("Not master, can't del deivce!");
        return -1;
    }
    
    if(num > DM_DEVICES_NUM_MAX)
    {
        return -1;
    }

    for(i = 0; i < num; i++)
    {
        device = DMDeviceAddressFind(addr[i]);

        if(device != NULL)
        {
            SysPrintf("Del dev:%d\n", addr[i]);
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

/*组网添加从设备*/
int8_t WMNetBuildAddDevice(uint8_t *id, uint8_t num)
{
    uint8_t i;
    uint8_t address[WM_SEARCH_DEVICE_NUM] = {0};
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
    if(g_mode != WM_MS_MODE_MASTER)
    {
        SysErrLog("Not master, can't add deivce!");
        return -1;
    }
    
    netBuildStatusSet(WM_STATUS_NET_BUILDING);
    NetBuildStart();  //切换到组网信道
    
    for(i = 0; i < num; i++)
    {
        if(id[i] < WM_SEARCH_DEVICE_NUM)
        {
            searchInfo = &g_searchDevInfo[id[i]];
            subAddr = DMDeviceUidToAddress(searchInfo->uid); 
            if(subAddr == DM_DEVICE_INVALID_FLAG) //第一次添加
            {
                keyVal = (uint8_t)getRandomValue(SysTime());//随机数产生 加密key
                subAddr = DMDeviceCreate(searchInfo->sleep, keyVal, searchInfo->uid, searchInfo->type);
            }
            else //之前添加过，直接用原来的key
            {
                keyVal = DMDeviceAddressFind(subAddr)->netInfo.key;
                DMDeviceSet(subAddr, keyVal, searchInfo->uid, searchInfo->type, searchInfo->sleep);//更新信息
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
            address[i] = subAddr; //记录地址，用于返回添加结果
            
            //随机分配发送心跳的起始时间，避免心跳冲突
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
    if(g_eventHandle != NULL)
    {
        g_eventHandle(WM_EVENT_ADD_RESULT, address);
    }
    SysTimerSet(netBuildDone, num * 600, 0, (void *)WM_EVENT_ADD_DEV_END); //等待添加消息发送完成后切换到工作信道
    return 0;
    
}

static void searchDeviceAgain(void *args)
{
    NetbuildSearchDevice(g_myDevType, wmSearchResultHandle);
    SysTimerSet(netBuildDone, 1000, 0, (void *)WM_EVENT_SEARCH_END);
}

/*唤醒后处理函数*/
void WMWakeupHandle(void)
{
#if 0
    if(g_isSleepDevice)
    {
        PMWakeUp();
        HalStatusLedSet(1); //blink 
        if(WMGetNetStatus() == WM_STATUS_NET_BUILDED)
        {
            NetSendHeartbeat(WM_MASTER_NET_ADDR); //send heartbeat
        }
        PMSetSleepStatus(false);//wakeup status
    }
#endif
}

/*搜索配网中的从设备*/
int8_t WMNetBuildSearch(void)
{
    SysLog("");
    if(g_netStatus == WM_STATUS_NET_BUILDING)
    {
        SysErrLog("wait, already in search ...");
        return -1;
    }

    if(g_mode == WM_MS_MODE_MASTER)
    {
        netBuildStatusSet(WM_STATUS_NET_BUILDING);
        #if 0
        if(g_netbuildInfo.rfChannel == 0xff) //随机分配通信信道
        {
            g_netbuildInfo.rfChannel = NET_WORK_START_CH + (getRandomValue(SysTime()) % 30);
        }
        #endif
        searchDeviceInfoClear(); //清除缓存
        NetBuildStart();         //切换到配网通道
        NetbuildSearchDevice(g_myDevType, wmSearchResultHandle);
        SysTimerSet(searchDeviceAgain, 1000, 0, NULL); //搜索两次
        return 0;
    }
    return -1;
}

/*进入退出配网*/
void WMNetBuildStart(bool start)
{
    SysLog("start = %d", start);
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

/*协调通信
* @isBuild: 0=断开，1=建立
* @addr1,addr2 要协调的两个从设备的地址
*/
int8_t WMDeviceCoordination(bool isBuild, uint8_t addr1, uint8_t addr2)
{
    uint8_t keyValue;
    NetCoordinationDev_t cooDev;
    DMDevicesInfo_t *dev1, *dev2;
    
    if(g_mode == WM_MS_MODE_MASTER)
    {
        dev1 = DMDeviceAddressFind(addr1);
        dev2 = DMDeviceAddressFind(addr2);
        if(dev1 == NULL || dev2 == NULL)
        {
            SysLog("can't find device");
            return -1;
        }
        keyValue = (uint8_t)getRandomValue(SysTime());
        
        cooDev.isBuild = isBuild;
        cooDev.key     = keyValue;
        cooDev.netAddr = addr1;
        cooDev.sleep   = dev1->netInfo.sleep;
        cooDev.uid     = dev1->netInfo.uid;
        memcpy(cooDev.devType, dev1->netInfo.devType, NET_DEV_TYPE_LEN);
        NetCoordinationOperate(addr2, dev2->netInfo.sleep, &cooDev); //将addr1的设备信息发给add2设备

        cooDev.isBuild = isBuild;
        cooDev.key     = keyValue;
        cooDev.netAddr = addr2;
        cooDev.sleep   = dev2->netInfo.sleep;
        cooDev.uid     = dev2->netInfo.uid;
        memcpy(cooDev.devType, dev2->netInfo.devType, NET_DEV_TYPE_LEN);
        NetCoordinationOperate(addr1, dev1->netInfo.sleep, &cooDev); //将addr2的设备信息发给add1设备
        
        SysLog("Build:%d, Dev:%d <--> Dev:%d", isBuild, addr1, addr2);
        return 0;
    }
    else
    {
        SysLog("Not master device!");
    }
    return -1;
}

/*设置主从模式*/
void WMSetMasterSlaveMode(bool isMaster)
{
    bool needSave = false;
    SysLog("isMaster = %d", isMaster);

    if(isMaster)
    {
        g_mode = WM_MS_MODE_MASTER;
        
        if(g_netbuildInfo.netAddr != NET_MASTER_NET_ADDR) //未进行配网
        {
            
            g_netbuildInfo.netAddr = NET_MASTER_NET_ADDR; //主设备网络地址始终为 0
            SysGetMacAddr(g_netbuildInfo.segAddr);  //主设备的MAC地址即为网络的段地址
            /*随机选择通信通道(90~120)*/
            SysRandomSeed(SysTime() + NetGetMacAddr(NULL)[3]);
            g_netbuildInfo.rfChannel = NET_WORK_START_CH + (SysRandom() % 30);
            
            needSave = true;
        }
        
        netBuildStatusSet(WM_STATUS_NET_BUILDED);   //主设备只有builded和building两种状态
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

/*清空配网信息及关联设备的信息*/
void WMNetInfoClear(void)
{
    SysLog("");
    DMDeviceInfoClear();
    HalFlashErase(SYS_NET_BUILD_INFO_ADDR);
    updateNetInfoAndSwitchChannel(false);
    if(g_mode == WM_MS_MODE_SLAVE)
    {
        netBuildStatusSet(WM_STATUS_NET_IDLE);
    }
}

void WMEventRegister(WMEventReport_cb eventHandle)
{
    g_eventHandle = eventHandle;
}

void WMInitialize(void)
{
    uint8_t mac[NET_MAC_ADDR_LEN] = {0x22, 0x02, 0x03, 0x55};
    //PMInitialize(); 
    NetLayerInit();
    NetLayerEventRegister(wmEventHandle);
#if 0
    SysSetMacAddr(mac);
#endif    
    NetSetMacAddr(SysGetMacAddr(mac));

    DMInitialize();
    DMEventRegister(dmEventHandle);

    HalStatusLedSet(3); //默认未配网

    memset(&g_netbuildInfo, 0xff, sizeof(WMNetbuildInfo_t));
    HalFlashRead(SYS_NET_BUILD_INFO_ADDR, &g_netbuildInfo, sizeof(WMNetbuildInfo_t));

    if(g_netbuildInfo.netAddr != 0xff) //已配网
    {
        netBuildStatusSet(WM_STATUS_NET_BUILDED);
        updateNetInfoAndSwitchChannel(false); //更新网络信息，切换工作信道
    }
}

void WMPoll(void)
{
    NetLayerPoll();
    subDevHeartbeat();
    DMPoll();
    #if 0
    if(g_isSleepDevice)
    {
        PMPoll();
    }
    #endif
}

