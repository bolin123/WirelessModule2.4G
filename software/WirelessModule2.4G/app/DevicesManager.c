#include "DevicesManager.h"

#define DM_DEVICE_ALLOCATE_FLAG 0xdd

static DMDevicesInfo_t g_deviceInfo[DM_DEVICES_NUM_MAX];
static DMEventHandle_cb g_eventHandle = NULL;

static void updateDeviceInfoToFlash(void)
{
    // TODO: save device info to flash
}

uint8_t DMDeviceUidToAddress(uint32_t uid)
{
    uint8_t i;

    for(i = 0; i < DM_DEVICES_NUM_MAX; i++)
    {
        if(g_deviceInfo[i].allocate == DM_DEVICE_ALLOCATE_FLAG \
            && uid == g_deviceInfo[i].netInfo.uid)
        {
            return i; //device address
        }
    }
    return DM_DEVICE_INVALID_FLAG;
}

DMDevicesInfo_t *DMDeviceAddressFind(uint8_t addr)
{
    if(addr > (DM_DEVICES_NUM_MAX - 1))
    {
        return NULL;
    }
    
    if(g_deviceInfo[addr].allocate == DM_DEVICE_ALLOCATE_FLAG)
    {
        return &g_deviceInfo[addr];
    }
    return NULL;
}

void DMDeviceDel(DMDevicesInfo_t *device)
{
    if(device != NULL)
    {
        memset(device, DM_DEVICE_INVALID_FLAG, sizeof(DMDevicesInfo_t));
        updateDeviceInfoToFlash();
    }
}

/*设置设备信息
* 
*/
void DMDeviceSet(uint8_t addr, uint8_t key, uint32_t uid, uint8_t *type, bool sleep)
{
    DMDevicesInfo_t *device = &g_deviceInfo[addr];
    
    device->allocate = DM_DEVICE_ALLOCATE_FLAG;
    device->netInfo.sleep = sleep;
    device->netInfo.key = key;
    device->netInfo.uid = uid;
    memcpy(device->netInfo.devType, type, NET_DEV_TYPE_LEN);
    memset(&device->hbInfo, 0, sizeof(DMDeviceNetInfo_t));

    updateDeviceInfoToFlash();
}

uint8_t DMDeviceCreate(bool sleep, uint8_t key, uint32_t uid, uint8_t *type)
{
    uint8_t i;

    //主设备地址为0，从设备分配从地址1开始
    for(i = 1; i < DM_DEVICES_NUM_MAX; i++)
    {
        if(g_deviceInfo[i].allocate != DM_DEVICE_ALLOCATE_FLAG)
        {
            g_deviceInfo[i].allocate = DM_DEVICE_ALLOCATE_FLAG;
            g_deviceInfo[i].netInfo.sleep = sleep;
            g_deviceInfo[i].netInfo.key = key;
            g_deviceInfo[i].netInfo.uid = uid;
            memcpy(g_deviceInfo[i].netInfo.devType, type, NET_DEV_TYPE_LEN);
            memset(&g_deviceInfo[i].hbInfo, 0, sizeof(DMDeviceNetInfo_t));
            
            updateDeviceInfoToFlash();
            return i;
        }
    }
    return DM_DEVICE_INVALID_FLAG;
}

void DMUpdateHeartbeat(uint8_t addr)
{
    DMDevicesInfo_t *info = DMDeviceAddressFind(addr);

    if(info != NULL)
    {
        if(!info->hbInfo.isOnline)
        {
            info->hbInfo.isOnline = true;
            g_eventHandle(addr, DM_EVENT_ONLINE);
        }
        info->hbInfo.lastHBTime = SysTime();
    }
    else
    {
        SysErrLog("device not found!!!");
    }
}

static void heartbeatPoll(void)
{
    static uint8_t address = 0;
    uint32_t hbTime;
    DMDevicesInfo_t *info = &g_deviceInfo[address];

    if(info->allocate == DM_DEVICE_ALLOCATE_FLAG && info->hbInfo.isOnline)
    {
        hbTime = info->netInfo.sleep ? NET_SLEEP_DEVICE_HBTIME : NET_NORMAL_DEVICE_HBTIME;
        if(SysTimeHasPast(info->hbInfo.lastHBTime, hbTime * 3)) // 3次未收到心跳，认为是掉线
        {
            info->hbInfo.isOnline = false;
            g_eventHandle(address, DM_EVENT_OFFLINE);
        }
    }
    address++;
    if(address >= DM_DEVICES_NUM_MAX)
    {
        address = 0;
    }
}

void DMEventRegister(DMEventHandle_cb eventHandle)
{
    g_eventHandle = eventHandle;
}

void DMPoll(void)
{
    heartbeatPoll();
}

void DMInitialize(void)
{
    //load flash
}

