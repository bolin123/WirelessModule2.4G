#include "MProto.h"
#include "VTList.h"
#include "WirelessModule.h"
#include "DevicesManager.h"
#include "HalUart.h"

#define MPROTO_FRAME_HEAD 0xFC

#define MPROTO_SEND_RETRY_TIME  3
#define MPROTO_SEND_TIMEOUT  200

typedef struct MProtoFrameSendList_st
{
    uint8_t retry;
    uint8_t dlen;
    uint8_t *data;
    SysTime_t lastSendTime;
    VTLIST_ENTRY(struct MProtoFrameSendList_st);
}MProtoFrameSendList_t;

typedef struct
{
    uint8_t head;    
    uint8_t type;
    uint8_t dlen;
    uint8_t check;
    uint8_t data[];
}MProtoFrameHead_t;

typedef struct
{
    uint8_t type[NET_DEV_TYPE_LEN];
    bool isMaster;
    bool needSleep;
}MProtoRequstDeviceInfo_t;

typedef struct
{
    uint8_t to;
    uint8_t dlen;
    uint8_t data[];
}MProtoUserData_t;

typedef enum
{
    MPROTO_FRAME_TYPE_ACK           = 0x00,  //应答
    MPROTO_FRAME_TYPE_REQUEST       = 0x01,  //请求设备信息
    MPROTO_FRAME_TYPE_NETCONFIG     = 0x02,  //组网
    MPROTO_FRAME_TYPE_SEARCH        = 0x03,  //查找从设备
    MPROTO_FRAME_TYPE_ADD_DEVICE    = 0x04,  //添加从设备
    MPROTO_FRAME_TYPE_DEL_DEVICE    = 0x05,  //删除从设备
    MPROTO_FRAME_TYPE_HEARTBEAT     = 0x06,  //心跳
    MPROTO_FRAME_TYPE_USER_DATA     = 0x07,  //用户数据
    MPROTO_FRAME_TYPE_ON_OFFLINE    = 0x08,  //上下线通知
    MPROTO_FRAME_TYPE_COORDINATION  = 0x09,  //协调通信
    MPROTO_FRAME_TYPE_QUERY_ONLINE  = 0x0a,  //查询在线设备
    MPROTO_FRAME_TYPE_QUERY_RELATED = 0x0b,  //查询关联设备
    MPROTO_FRAME_TYPE_DATA_CLEAR    = 0x0c,  //清除配网数据
}MProtoFrameType_t;

static uint8_t g_byteBuff[255];
static uint8_t g_byteCount = 0;
static MProtoFrameSendList_t g_frameSendList;
static bool g_gotDeviceInfo = false;

static uint8_t checksum(const void *buf, uint16_t len)
{
    const uint8_t *data = buf;
    uint8_t sum = 0;
    uint16_t i;
    for(i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return sum;
}

static void lowDataSend(const uint8_t *data, uint8_t len)
{
    uint8_t i;
    
    SysPrintf("MProto send:");
    for(i = 0; i < len; i++)
    {
        SysPrintf("%02x ", data[i]);
    }
    SysPrintf("\n");
    HalUartWrite(HAL_UART_1, data, len);
}

static void mprotoFrameSend(MProtoFrameType_t type, bool needAck, uint8_t *data, uint8_t dlen)
{
    uint8_t buff[255];
    MProtoFrameHead_t *frame = (MProtoFrameHead_t *)buff;
    
    frame->head = MPROTO_FRAME_HEAD;
    frame->type = type | (needAck ? 0x80 : 0x00);
    frame->dlen = dlen;
    frame->check = checksum(data, dlen);
    memcpy(frame->data, data, dlen);

    if(needAck)
    {
        MProtoFrameSendList_t *sendList = (MProtoFrameSendList_t *)malloc(sizeof(MProtoFrameSendList_t));

        sendList->retry = MPROTO_SEND_RETRY_TIME;
        sendList->lastSendTime = 0;
        sendList->dlen = dlen + sizeof(MProtoFrameHead_t);
        sendList->data = (uint8_t *)malloc(sendList->dlen);
        memcpy(sendList->data, buff, sendList->dlen);
        VTListAdd(&g_frameSendList, sendList);
    }
    else
    {
        lowDataSend(buff, dlen + sizeof(MProtoFrameHead_t));
    }
}

static void sendAck(void)
{
    mprotoFrameSend(MPROTO_FRAME_TYPE_ACK, false, NULL, 0);
}

static void coordinationResult(uint8_t build, bool success, uint8_t addr1, uint8_t addr2)
{
    uint8_t data[4];

    data[0] = build;
    data[1] = (success ? 0 : 1);
    data[2] = addr1;
    data[3] = addr2;
    
    if(success)
    {
        mprotoFrameSend(MPROTO_FRAME_TYPE_COORDINATION, false, data, sizeof(data));
    }
    else
    {
        mprotoFrameSend(MPROTO_FRAME_TYPE_COORDINATION, false, data, sizeof(data));
    }
}

static void mprotoFrameHandle(MProtoFrameType_t type, bool needAck, uint8_t *data, uint8_t dlen)
{
    uint8_t buff[255];
    uint8_t len, i;

    if(needAck && type != MPROTO_FRAME_TYPE_ACK)
    {
        sendAck();
    }

    switch(type)
    {
    case MPROTO_FRAME_TYPE_ACK:
        {
            MProtoFrameSendList_t *sendList = VTListFirst(&g_frameSendList);
            if(sendList != NULL)
            {
                VTListDel(sendList);
                free(sendList->data);
                free(sendList);
            }
        }
        break;
    case MPROTO_FRAME_TYPE_REQUEST:
        {
            MProtoRequstDeviceInfo_t *deviceInfo = (MProtoRequstDeviceInfo_t *)data;
            WMSetModuleType(deviceInfo->type);
            WMSetMasterSlaveMode(deviceInfo->isMaster);
            WMSetSleepMode(deviceInfo->needSleep);
            g_gotDeviceInfo = true;
        }
        break;
    case MPROTO_FRAME_TYPE_NETCONFIG:
        WMNetBuildStart(data[0]);
        break;
    case MPROTO_FRAME_TYPE_SEARCH:
        WMNetBuildSearch();
        break;
    case MPROTO_FRAME_TYPE_ADD_DEVICE:
        WMNetBuildAddDevice(data, dlen);
        break;
    case MPROTO_FRAME_TYPE_DEL_DEVICE:
        WMNetBuildDelDevice(data, dlen);
        break;
    //case MPROTO_FRAME_TYPE_HEARTBEAT: //ignor
    //    break;
    case MPROTO_FRAME_TYPE_USER_DATA:
        {
            MProtoUserData_t *userData = (MProtoUserData_t *)data;
            WMNetUserDataSend(userData->to, userData->data, userData->dlen);
        }
        break;
    //case MPROTO_FRAME_TYPE_ON_OFFLINE: //ignor
    //    break;
    case MPROTO_FRAME_TYPE_COORDINATION:
        if(WMDeviceCoordination(data[0], data[1], data[2]) < 0)
        {
            coordinationResult(data[0], false, data[1], data[2]);
        }
        else
        {
            coordinationResult(data[0], true, data[1], data[2]);
        }
        break;
    case MPROTO_FRAME_TYPE_QUERY_ONLINE:
        len = WMQueryDeviceInfo(WMQUERY_TYPE_ONLINE, buff);
        mprotoFrameSend(MPROTO_FRAME_TYPE_QUERY_ONLINE, false, buff, len);
        break;
    case MPROTO_FRAME_TYPE_QUERY_RELATED:
        len = WMQueryDeviceInfo(WMQUERY_TYPE_RELATED, buff);
        mprotoFrameSend(MPROTO_FRAME_TYPE_QUERY_RELATED, false, buff, len);
        break;
    case MPROTO_FRAME_TYPE_DATA_CLEAR:
        WMNetInfoClear();
        break;
    default:
        break;
    }
}

void MProtoRecvByte(uint8_t byte)
{
    MProtoFrameHead_t *frame;
    static uint8_t dataLength = 0;
    static SysTime_t lastRecvTime = 0;

    if(SysTimeHasPast(lastRecvTime, 100))
    {
        g_byteCount = 0;
    }
    lastRecvTime = SysTime();

    g_byteBuff[g_byteCount++] = byte;

    if(g_byteCount == 1)
    {
        if(byte != MPROTO_FRAME_HEAD)
        {
            g_byteCount = 0;
        }
    }
    else if(g_byteCount == 3)
    {
        dataLength = byte;
    }
    else if(g_byteCount == dataLength + sizeof(MProtoFrameHead_t))
    {
        frame = (MProtoFrameHead_t *)g_byteBuff;

        if(frame->check == checksum(frame->data, frame->dlen))
        {
            uint8_t i;
            SysPrintf("MProto recv:");
            for(i = 0; i < g_byteCount; i++)
            {
                SysPrintf("%02x ", g_byteBuff[i]);
            }
            SysPrintf("\n");
            mprotoFrameHandle((frame->type & 0x7f), (frame->type & 0x80), frame->data, frame->dlen);
        }
        g_byteCount = 0;
    }
}

static void mprotoSendListHandle(void)
{
    MProtoFrameSendList_t *sendList = VTListFirst(&g_frameSendList);

    if(sendList != NULL)
    {
        if(SysTimeHasPast(sendList->lastSendTime, MPROTO_SEND_TIMEOUT))
        {
            if(sendList->retry)
            {
                lowDataSend(sendList->data, sendList->dlen);
                sendList->retry--;
                sendList->lastSendTime = SysTime();
            }
            else
            {
                VTListDel(sendList);
                free(sendList->data);
                free(sendList);
            }
        }
    }
}

static void requestDeviceInfo(void)
{
    static SysTime_t lastRequestTime = 0;

    if(!g_gotDeviceInfo)
    {
        if(lastRequestTime == 0 || SysTimeHasPast(lastRequestTime, 1000))
        {
            mprotoFrameSend(MPROTO_FRAME_TYPE_REQUEST, false, NULL, 0);
            lastRequestTime = SysTime();
        }
    }
}

static bool netStatusChanged(void)
{
    uint8_t status;
    static uint8_t oldStatus;
    
    status = (WMGetNetStatus() << 1) + WMIsDeviceOnline();

    if(status != oldStatus)
    {
        oldStatus = status;
        return true;
    }
    
    return false;
}

static void heartbeatSend(void)
{
    static SysTime_t lastHbTime = 0;
    uint8_t status;

    if(lastHbTime == 0 || SysTimeHasPast(lastHbTime, 5000) || netStatusChanged())
    {
        status = (WMGetNetStatus() << 1) + WMIsDeviceOnline();
        mprotoFrameSend(MPROTO_FRAME_TYPE_HEARTBEAT, false, &status, sizeof(status));

        if(lastHbTime == 0)
        {
            netStatusChanged(); //更新一次oldStatus
        }
        lastHbTime = SysTime();
    }
}

static void mprotoEventHandle(WMEvent_t event, void *args)
{
    uint8_t i;
    uint8_t num = 0;
    uint8_t data[254];
    uint8_t *result;
    DMDevicesInfo_t *devInfo;
    
    switch(event)
    {
        case WM_EVENT_ONLINE:      //设备上线
            devInfo = DMDeviceAddressFind((uint8_t)((uint32_t)args));
            if(devInfo != NULL)
            {
                data[num++] = 1; //online
                data[num++] = (uint8_t)((uint32_t)args);
                memcpy(&data[num], devInfo->netInfo.devType, NET_DEV_TYPE_LEN);
                num += NET_DEV_TYPE_LEN;
                data[num++] = devInfo->netInfo.sleep;
                mprotoFrameSend(MPROTO_FRAME_TYPE_ON_OFFLINE, true, data, num);
            }
            break;
        case WM_EVENT_OFFLINE:     //设备下线
            devInfo = DMDeviceAddressFind((uint8_t)((uint32_t)args));
            if(devInfo != NULL)
            {
                data[num++] = 0; //offline
                data[num++] = (uint8_t)((uint32_t)args);
                memcpy(&data[num], devInfo->netInfo.devType, NET_DEV_TYPE_LEN);
                num += NET_DEV_TYPE_LEN;
                data[num++] = devInfo->netInfo.sleep;
                mprotoFrameSend(MPROTO_FRAME_TYPE_ON_OFFLINE, true, data, num);
            }
            break;
        case WM_EVENT_SEARCH_END:  //搜索结束
            num = WMGetSearchResult(data);
            mprotoFrameSend(MPROTO_FRAME_TYPE_SEARCH, true, data, num);
            break;
        case WM_EVENT_ADD_RESULT: //添加结果，返回设备网络地址和信息
            result = (uint8_t *)args;
            for(i = 0; result[i] != 0; i++)
            {
                devInfo = DMDeviceAddressFind(result[i]);
                if(devInfo != NULL)
                {
                    data[num++] = result[i];
                    memcpy(&data[num], devInfo->netInfo.devType, NET_DEV_TYPE_LEN);
                    num += NET_DEV_TYPE_LEN;
                    data[num++] = devInfo->netInfo.sleep;
                }
            }
            mprotoFrameSend(MPROTO_FRAME_TYPE_ADD_DEVICE, true, data, num);
            break;
        case WM_EVENT_ADD_DEV_END: //添加结束
            break;
        case WM_EVENT_RECV_SEARCH: //接收到搜索消息
            break;
        case WM_EVENT_NET_STATUS_CHANGE: //状态改变
            break;
        case WM_EVENT_COORDINATION: //协调通信
            mprotoFrameSend(MPROTO_FRAME_TYPE_COORDINATION, true, (uint8_t *)args, sizeof(WMCoordinationData_t));
            break;
        case WM_EVENT_USER_DATA:
            {
                WMUserData_t *userData = (WMUserData_t *)args;
                data[num++] = userData->from;
                data[num++] = userData->isBroadcast;
                data[num++] = userData->dlen;
                memcpy(&data[num], userData->data, userData->dlen);
                num += userData->dlen;
                mprotoFrameSend(MPROTO_FRAME_TYPE_USER_DATA, true, data, num);
            }
            break;
        default:
            break;
    }
}

static void commPortInit(void)
{
    HalUartConfig_t uartConfig;
    uartConfig.baudRate = 9600;
    uartConfig.parity = PARITY_NONE;
    HalUartInit(HAL_UART_1, &uartConfig);
}

void MProtoInitialize(void)
{
    VTListInit(&g_frameSendList);
    WMInitialize();
    WMEventRegister(mprotoEventHandle);
    commPortInit();
}

void MProtoPoll(void)
{
    requestDeviceInfo();
    mprotoSendListHandle();
    if(g_gotDeviceInfo)
    {
       WMPoll(); 
       heartbeatSend();
    }
}
