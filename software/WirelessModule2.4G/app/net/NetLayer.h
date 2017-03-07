#ifndef NET_LAYER_H
#define NET_LAYER_H

#include "HalCtype.h"
#include "PHYLayer.h"

#define NET_DEV_TYPE_LEN 6
#define NET_SEG_ADDR_LEN PHY_MAC_LEN
#define NET_MAC_ADDR_LEN PHY_MAC_LEN

#define NET_BUILD_CH_NUM 0x7D //配网通道 125
#define NET_WORK_START_CH 0x5A  //通信信道 0x5a ~ 0x78(90 ~ 120)

#define NET_MASTER_NET_ADDR 0x00
#define NET_BROADCAST_NET_ADDR 0xff

#define NET_SLEEP_DEVICE_HBTIME (300000) //5min
#define NET_NORMAL_DEVICE_HBTIME (5000) //5s

//typedef enum
//{
//    NET_MODE_NONE = 0,
//    NET_MODE_SLAVE,
//    NET_MODE_MASTER,
//}NetMode_t;


typedef enum
{
    NET_EVENT_SEARCH = 0,   //搜索设备
    NET_EVENT_DEV_INFO,     //搜索过程中上报的从设备信息
    NET_EVENT_DEV_ADD,      //添加设备
    NET_EVENT_DEV_DEL,      //删除设备
    NET_EVENT_COORDINATION, //协调通信
    NET_EVENT_DEVICE_HEARTBEAT, //心跳
    NET_EVENT_USER_DATA,    //用户数据
}NetEventType_t;

/*组网时从设备上报的信息*/
typedef struct
{
    uint8_t type[NET_DEV_TYPE_LEN];
    uint8_t sleep;
}NetbuildSubDeviceInfo_t;
typedef void (* NetSearchInfoHandle_cb)(uint32_t uid, NetbuildSubDeviceInfo_t *result); //搜索结果回调函数

/*组网添加设备信息*/
typedef struct
{
    uint8_t addr;
    uint8_t rfChannel;
    uint8_t key;
    uint8_t hbTime[2];
    uint8_t masterType[NET_DEV_TYPE_LEN];
}NetbuildAddSubInfo_t;

typedef struct
{
    bool isBuild;
    uint8_t key;
    uint8_t netAddr;
    uint8_t devType[NET_DEV_TYPE_LEN];
    uint8_t sleep;
    uint32_t uid;
}NetCoordinationDev_t;

typedef struct
{
    bool isBroadcast;
    uint8_t dlen;
    uint8_t *data;
}NetUserData_t;

typedef struct
{
    uint8_t devType[NET_DEV_TYPE_LEN];
    uint8_t sleep;
}NetbuildSubReportInfo_t;

typedef void(*NetEventHandle_cb)(NetEventType_t event, uint32_t from, void *args);

//master module interface
void NetbuildSearchDevice(const uint8_t myType[NET_DEV_TYPE_LEN], NetSearchInfoHandle_cb searchHandle);
void NetbuildAddDevice(uint32_t uid, NetbuildAddSubInfo_t *subInfo);
void NetCoordinationOperate(uint8_t to, bool sleep, NetCoordinationDev_t *cooDev);
void NetbuildDelDevice(uint8_t addr, bool isSleep);

//slave module interface
void NetbuildSearchResponse(const uint8_t *devType, bool isSleep);
//void NetbuildSubReportDevInfo(uint32_t masterUID, NetbuildSubReportInfo_t *devInfo);
void NetSendHeartbeat(uint8_t addr);

void NetUserDataSend(uint8_t to, bool isSleep, uint8_t *data, uint8_t len);

//config net args
void NetSetMacAddr(const uint8_t *mac);
uint8_t *NetGetMacAddr(uint8_t *mac);
//void NetSetModuleType(const uint8_t type[NET_DEV_TYPE_LEN]);
void NetSetSleepMode(bool needSleep);
//void NetSetModuleMode(NetMode_t mode);
void NetSetSegaddress(const uint8_t *segAddr);
void NetSetNetaddress(uint8_t addr);

void NetBuildStart(void);
void NetBuildStop(uint8_t workCh);
void NetLayerSleep(bool sleep);
bool NetSendListEmpty(void);

void NetLayerInit(void);
void NetLayerPoll(void);
void NetLayerEventRegister(NetEventHandle_cb eventCb);

#endif
