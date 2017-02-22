#ifndef DEVICES_MANAGER_H
#define DEVICES_MANAGER_H

#include "Sys.h"
#include "NetLayer.h"

#define DM_DEVICES_NUM_MAX 32
#define DM_DEVICE_INVALID_FLAG 0xff

typedef enum
{
    DM_EVENT_ONLINE,
    DM_EVENT_OFFLINE,   
}DMEvent_t;

//设备网络信息，需要存储
typedef struct
{
//    uint8_t addr;
    uint8_t sleep;
    uint8_t devType[NET_DEV_TYPE_LEN];
    uint8_t key;
    uint32_t uid;
}DMDeviceNetInfo_t;

typedef struct
{
    bool allocate;
    DMDeviceNetInfo_t netInfo;

    //心跳信息
    struct
    {
        bool isOnline;
        SysTime_t lastHBTime;
    }hbInfo;
}DMDevicesInfo_t;

typedef void (* DMEventHandle_cb)(uint8_t addr, DMEvent_t event);

void DMDeviceDel(DMDevicesInfo_t *device);
void DMDeviceSet(uint8_t addr, uint8_t key, uint32_t uid, uint8_t *type, bool sleep);
uint8_t DMDeviceCreate(bool sleep, uint8_t key, uint32_t uid, uint8_t *type);
uint8_t DMDeviceUidToAddress(uint32_t uid);
DMDevicesInfo_t *DMDeviceAddressFind(uint8_t addr);
void DMUpdateHeartbeat(uint8_t addr);
void DMEventRegister(DMEventHandle_cb eventHandle);
void DMPoll(void);
void DMInitialize(void);

#endif

