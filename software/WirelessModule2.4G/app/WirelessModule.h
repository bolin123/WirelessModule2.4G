#ifndef WIRELESS_MODULE_H
#define WIRELESS_MODULE_H

#include "Sys.h"
#include "NetLayer.h"

#define WM_NET_BROADCAST_ADDR 0xff
#define WM_SEARCH_DEVICE_NUM 5
#define WM_MASTER_NET_ADDR NET_MASTER_NET_ADDR

typedef enum
{
    WM_EVENT_ONLINE,      //设备上线
    WM_EVENT_OFFLINE,     //设备下线
    WM_EVENT_SEARCH_END,  //搜索结束
    WM_EVENT_ADD_DEV_END, //添加结束
    WM_EVENT_RECV_SEARCH, //接收到搜索消息
    WM_EVENT_ADD_INFO,    //
    WM_EVENT_NET_STATUS_CHANGE, //状态改变
    WM_EVENT_COORDINATION, //协调通信
}WMEvent_t;

typedef enum
{
    WM_STATUS_NET_IDLE = 0,
    WM_STATUS_NET_BUILDING,
    WM_STATUS_NET_BUILDED,
}WMNetStatus_t;

typedef enum
{
    WMQUERY_TYPE_ONLINE = 0,
    WMQUERY_TYPE_RELATED,
}WMQueryType_t;

typedef struct
{
    uint8_t sid;
    bool sleep;
    uint8_t type[NET_DEV_TYPE_LEN];
}WMSearchResult_t;

typedef void(*WMEventReport_cb)(WMEvent_t event, void *args);

void WMNetUserDataSend(uint8_t to, uint8_t *data, uint8_t len);

uint8_t WMQueryDeviceInfo(WMQueryType_t type, uint8_t *contents);
uint8_t WMGetSearchResult(WMSearchResult_t result[WM_SEARCH_DEVICE_NUM]);
WMNetStatus_t WMGetNetStatus(void);

int8_t WMNetBuildDelDevice(uint8_t *addr, uint8_t num);
int8_t WMNetBuildAddDevice(uint8_t *id, uint8_t num);
int8_t WMNetBuildSearch(void);
void WMNetBuildStart(bool start);
int8_t WMDeviceCoordination(bool isBuild, uint8_t addr1, uint8_t addr2);

void WMSetMasterSlaveMode(bool isMaster);
void WMSetSleepMode(bool isSleep);
void WMSetModuleType(const uint8_t *type);

void WMEventRegister(WMEventReport_cb eventHandle);
void WMInitialize(void);
void WMPoll(void);

#endif
