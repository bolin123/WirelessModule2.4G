#include "MProto.h"
#include "VTList.h"
#include "WirelessModule.h"

#define MPROTO_FRAME_HEAD 0xFC

#define MPROTO_SEND_RETRY_TIME  3
#define MPROTO_SEND_TIMEOUT  500

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
    uint8_t data[0];
}MProtoFrameHead_t;

typedef struct
{
    uint8_t type[MPROTO_DEVICE_TYPE_LEN];
    bool isMaster;
    bool needSleep;
}MProtoRequstDeviceInfo_t;

typedef struct
{
    uint8_t to;
    uint8_t dlen;
    uint8_t data[0];
}MProtoUserData_t;

typedef enum
{
    MPROTO_FRAME_TYPE_ACK           = 0x00,
    MPROTO_FRAME_TYPE_REQUEST       = 0x01,
    MPROTO_FRAME_TYPE_NETCONFIG     = 0x02,
    MPROTO_FRAME_TYPE_SEARCH        = 0x03,
    MPROTO_FRAME_TYPE_ADD_DEVICE    = 0x04,
    MPROTO_FRAME_TYPE_DEL_DEVICE    = 0x05,
    MPROTO_FRAME_TYPE_HEARTBEAT     = 0x06,
    MPROTO_FRAME_TYPE_USER_DATA     = 0x07,
    MPROTO_FRAME_TYPE_ON_OFFLINE    = 0x08,
    MPROTO_FRAME_TYPE_COORDINATION  = 0x09,
    MPROTO_FRAME_TYPE_QUERY_ONLINE  = 0x0a,
    MPROTO_FRAME_TYPE_QUERY_RELATED = 0x0b,
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

static void mprotoFrameHandle(MProtoFrameType_t type, bool needAck, uint8_t *data, uint8_t dlen)
{
    uint8_t buff[255];
    uint8_t len;

    if(needAck && type != MPROTO_FRAME_TYPE_ACK)
    {
        //send ack
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
        WMNetBuildStateSet(data[0]);
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
    case MPROTO_FRAME_TYPE_HEARTBEAT:
        break;
    case MPROTO_FRAME_TYPE_USER_DATA:
    {
        MProtoUserData_t *userData = (MProtoUserData_t *)data;
        WMNetUserDataSend(userData->to, userData->data, userData->dlen);
    }
        break;
    case MPROTO_FRAME_TYPE_ON_OFFLINE:
        break;
    case MPROTO_FRAME_TYPE_COORDINATION:
        WMDeviceCoordination(data[0], data[1], data[2]);
        break;
    case MPROTO_FRAME_TYPE_QUERY_ONLINE:
        len = WMQueryDeviceInfo(WMQUERY_TYPE_ONLINE, buff);
        //TODO: send
        break;
    case MPROTO_FRAME_TYPE_QUERY_RELATED:
        len = WMQueryDeviceInfo(WMQUERY_TYPE_RELATED, buff);
        //TODO: send
        break;
    default:
        break;
    }
}

void MProtoRecvByte(uint8_t byte)
{
    MProtoFrameHead_t *frame;
    static uint8_t dataLength = 0;

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

    if(!g_gotDeviceInfo && SysTimeHasPast(lastRequestTime, 1000))
    {
        mprotoFrameSend(MPROTO_FRAME_TYPE_REQUEST, false, NULL, 0);
        lastRequestTime = SysTime();
    }
}

void MProtoInitialize(void)
{
    VTListInit(&g_frameSendList);
}

void MProtoPoll(void)
{
    mprotoSendListHandle();
}
