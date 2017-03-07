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
    WM_EVENT_ADD_RESULT,
    WM_EVENT_ADD_DEV_END, //添加结束
    WM_EVENT_RECV_SEARCH, //接收到搜索消息
    WM_EVENT_NET_STATUS_CHANGE, //状态改变
    WM_EVENT_COORDINATION, //协调通信
    WM_EVENT_USER_DATA,   //用户数据
    WM_EVENT_DELETED,     //设备被删除
}WMEvent_t;

typedef enum
{
    WM_STATUS_NET_IDLE     = 0,
    WM_STATUS_NET_BUILDING = 1,
    WM_STATUS_NET_BUILDED  = 2,
}WMNetStatus_t;

typedef enum
{
    WMQUERY_TYPE_ONLINE = 0,
    WMQUERY_TYPE_RELATED,
}WMQueryType_t;

typedef struct
{
    uint8_t sid;
    uint8_t type[NET_DEV_TYPE_LEN];
    bool sleep;
}WMSearchResult_t;

typedef struct
{
    uint8_t build;
    uint8_t address;
    uint8_t type[NET_DEV_TYPE_LEN];
    uint8_t sleep;
}WMCoordinationData_t;

typedef struct
{
    uint8_t from;
    uint8_t isBroadcast;
    uint8_t dlen;
    uint8_t *data;
}WMUserData_t;

typedef void(*WMEventReport_cb)(WMEvent_t event, void *args);

void WMNetUserDataSend(uint8_t to, uint8_t *data, uint8_t len);

uint8_t WMQueryDeviceInfo(WMQueryType_t type, uint8_t *contents);
uint8_t WMGetSearchResult(uint8_t *result);
WMNetStatus_t WMGetNetStatus(void);
bool WMIsDeviceOnline(void);

void WMNetInfoClear(void);
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
