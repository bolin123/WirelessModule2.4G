#include "Manufacture.h"
#include "HalUart.h"
#include "Netlayer.h"
#include "SysTimer.h"

#define MF_FRAME_HEAD_FLAG 0xaa

/*-------------------
* 0x00 通信测试
* 0x01 写入模块信息
* 0x02 读取模块信息
* 0x03 模块测试
* 0x04 模块测试结果
--------------------*/
typedef enum
{
    MF_TYPE_COMM_TEST = 0x00,
    MF_TYPE_WRITE_INFO,
    MF_TYPE_READ_INFO,
    MF_TYPE_START_CHECK,
    MF_TYPE_CHECK_RESULT,
}MFFrameType_t;

typedef struct
{
    uint8_t head;
    uint8_t len;
    uint8_t crc;
    uint8_t type;
    uint8_t data[];
}MFFrame_t;

typedef struct
{
    uint8_t testType;
    uint8_t testRes;
    uint8_t testFailReason;
    uint8_t msg[];
}MFTestResp_t;


static uint8_t g_buff[128];
static uint8_t g_count = 0;
static uint8_t g_testType = 0;
static int8_t g_testResult = 0;
static bool g_sendCheckResult = false;

static uint8_t checkSum(uint8_t *data, uint8_t len)
{
    uint8_t i, sum;

    for(i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return sum;
}

static void mfFrameSend(uint8_t type, uint8_t *data, uint8_t len)
{
    uint8_t buf[128];
    MFFrame_t *frame = (MFFrame_t *)buf;
    frame->head = MF_FRAME_HEAD_FLAG;
    frame->len = 2 + len;
    frame->type = type;
    memcpy(frame->data, data, len);
    frame->crc = checkSum(&frame->crc + 1, frame->len - 1);
#if 0

    uint8_t i;
    for(i = 0; i < frame->len + 2; i++)
    {
        SysPrintf("%02x ", buf[i]);
    }
    SysPrintf("\n");
#endif
    HalUartWrite(HAL_UART_1, buf, frame->len + 2);
}

static void mfSendTestResult(void)
{
    if(!g_sendCheckResult)
    {
        return ;
    }
    
    uint8_t data[10];
    char *msg = NULL;
    MFTestResp_t *resp = (MFTestResp_t *)data;

    resp->testType = g_testType;
    resp->testRes  = g_testResult > 0 ? true : false;
    resp->testFailReason = 1;
    if(g_testResult == 1)
    {
        msg = "test success.";
    }
    else if(g_testResult == -1)
    {
        msg = "can't find RF sensor!";
    }
    else if(g_testResult == -2)
    {
        msg = "scan timeout!";
    }
    strcpy((char *)resp->msg, msg);
    mfFrameSend(MF_TYPE_CHECK_RESULT, data, sizeof(MFTestResp_t) + strlen(msg));
}

static void mfSearchResultHandle(uint32_t uid, NetbuildSubDeviceInfo_t *result)
{
    g_sendCheckResult = true;
    g_testResult = 1;
}

static void testTimeoutHandle(void *args)
{
    g_sendCheckResult = true;
    g_testResult = -2;
}

static void searchDevice(void *args)
{
    if(g_testResult == 0)
    {
        NetbuildSearchDevice("ZCTEST", mfSearchResultHandle);
        SysTimerSet(testTimeoutHandle, 1000, 0, NULL);
    }
}

static void mfStartSelfCheck(uint8_t type)
{
    g_testType = type;
    g_testResult = 0;
    if(!NRF24L01Check())
    {
        g_testResult = -1;
        g_sendCheckResult = true;
    }
    else
    {
        NetBuildStart();
        NetbuildSearchDevice("ZCTEST", mfSearchResultHandle);
        SysTimerSet(searchDevice, 1000, 0, NULL);
    }
}

static void mfFrameHandle(MFFrame_t *frame)
{
    uint8_t data[64];
    switch(frame->type)
    {
        case MF_TYPE_COMM_TEST:
            mfFrameSend(MF_TYPE_COMM_TEST, NULL, 0);
            break;
        case MF_TYPE_WRITE_INFO:
            SysSetMacAddr(frame->data);
            mfFrameSend(MF_TYPE_WRITE_INFO, SysGetMacAddr(data), 4);
            break;
        case MF_TYPE_READ_INFO:
            memcpy(data, SysGetVersion(), 4);
            SysGetMacAddr(data + 4);
            mfFrameSend(MF_TYPE_READ_INFO, data, 8);
            break;
        case MF_TYPE_START_CHECK:
            mfFrameSend(MF_TYPE_START_CHECK, NULL, 0);
            mfStartSelfCheck(data[0]);
            break;
        case MF_TYPE_CHECK_RESULT:
            g_sendCheckResult = false;
            g_testResult = 0;
            break;
        default:
            break;
    }
}

void MFRecvByte(uint8_t byte)
{
    static uint8_t frameLength = 0;
    MFFrame_t *frame;
    
    g_buff[g_count++] = byte;

    if(g_count == 1)
    {
        if(byte != MF_FRAME_HEAD_FLAG)
        {
            g_count = 0;
        }
    }
    else if(g_count == 2)
    {
        if(byte > sizeof(g_buff) - 2)
        {
            g_count = 0;
            frameLength = 0;
        }
    }
    else if(g_count == frameLength + 2)
    {
        frame = (MFFrame_t *)g_buff;
        if(checkSum(&g_buff[2], frameLength) == frame->crc)
        {
            mfFrameHandle(frame);
        }
        g_count = 0;
        frameLength = 0;
    }
}

void MFPoll(void)
{
    mfSendTestResult();
}

void MFInitialize(void)
{
}

