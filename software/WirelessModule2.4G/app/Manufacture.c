#include "Manufacture.h"
#include "HalUart.h"
#include "Netlayer.h"
#include "SysTimer.h"

#define MF_FRAME_HEAD_FLAG 0xaa
#define MF_FRAME_BUFF_LEN  255

#define MF_ERR_CHECK_SUCCESS   1
#define MF_ERR_NOT_READY       0
#define MF_ERR_NO_MAC         -1
#define MF_ERR_PHY_NOT_FOUND  -2
#define MF_ERR_NOT_FOUND_DEV  -3

/*-------------------
* 0x00 通信测试
* 0x01 写入模块信息
* 0x02 读取模块信息
* 0x03 模块测试
* 0x04 模块测试结果
* 0xff 解锁
--------------------*/
typedef enum
{
    MF_TYPE_COMM_TEST = 0x00,
    MF_TYPE_WRITE_INFO,
    MF_TYPE_READ_INFO,
    MF_TYPE_START_CHECK,
    MF_TYPE_CHECK_RESULT,
    MF_TYPE_UNLOCK = 0xff,
}MFFrameType_t;

typedef enum
{
    INFO_EID_ID = 1,        //ID
    INFO_EID_PIN = 2,       //PIN码
    INFO_EID_MAC = 3,       //MAC地址
    INFO_EID_VER = 4,
    INFO_EID_DEV_TYPE = 5,
}MFInfoElementID_t;

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

typedef struct
{
    uint8_t verEid;
    uint8_t verLen;
    uint8_t version[4];
    uint8_t macEid;
    uint8_t macLen;
    uint8_t mac[PHY_MAC_LEN];
}MFRespInfo_t;

typedef struct
{
    uint8_t macEid;
    uint8_t macLen;
    uint8_t mac[PHY_MAC_LEN];
}MFWriteInfo_t;

static uint8_t g_count = 0;
static uint8_t g_testType = 0;
static int8_t g_testResult = 0;
static bool g_sendCheckResult = false;

static bool g_start = false;
static uint8_t *g_frameBuf = NULL;
static MFSendDataFunc_t g_sendDataFunc = NULL;

static uint8_t checkSum(uint8_t *data, uint8_t len)
{
    uint8_t i, sum = 0;

    for(i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return ~sum;
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
#if 1

    uint8_t i;
    for(i = 0; i < frame->len + 2; i++)
    {
        SysPrintf("%02x ", buf[i]);
    }
    SysPrintf("\n");
#endif
    if(g_sendDataFunc != NULL)
    {
        g_sendDataFunc(buf, frame->len + 2);
    }
}

static void mfSendTestResult(void)
{
    if(!g_sendCheckResult)
    {
        return ;
    }
    
    uint8_t data[10];
    char *msg = NULL;
    static SysTime_t lastSendTime = 0;
    MFTestResp_t *resp = (MFTestResp_t *)data;

    if(lastSendTime == 0 || SysTimeHasPast(lastSendTime, 200))
    {
        resp->testType = g_testType;
        resp->testRes  = g_testResult > 0 ? true : false;
        resp->testFailReason = 1;
        if(g_testResult == MF_ERR_CHECK_SUCCESS)
        {
            msg = "test success.";
        }
        else if(g_testResult == MF_ERR_PHY_NOT_FOUND)
        {
            msg = "can't find RF sensor!";
        }
        else if(g_testResult == MF_ERR_NOT_FOUND_DEV)
        {
            msg = "scan timeout!";
        }
        else if(g_testResult == MF_ERR_NO_MAC)
        {
            msg = "no MAC address!";
        }
            
        strcpy((char *)resp->msg, msg);
        mfFrameSend(MF_TYPE_CHECK_RESULT, data, sizeof(MFTestResp_t) + strlen(msg));
    }   
}

static void mfSearchResultHandle(uint32_t uid, NetbuildSubDeviceInfo_t *result)
{
    SysLog("");
    g_sendCheckResult = true;
    g_testResult = MF_ERR_CHECK_SUCCESS;
    NetBuildStop(0xff);
}

static void testTimeoutHandle(void *args)
{
    if(g_testResult == MF_ERR_NOT_READY)
    {
        g_sendCheckResult = true;
        g_testResult = MF_ERR_NOT_FOUND_DEV;
    }
    
}

static void searchDevice(void *args)
{
    if(g_testResult == MF_ERR_NOT_READY)
    {
        NetbuildSearchDevice("ZCTEST", mfSearchResultHandle);
        SysTimerSet(testTimeoutHandle, 2000, 0, NULL);
    }
}

static void mfStartSelfCheck(uint8_t type)
{
    uint8_t mac[PHY_MAC_LEN];
    
    g_testType = type;
    g_testResult = MF_ERR_NOT_READY;

    SysGetMacAddr(mac);
    if(mac[0] == 0xff \
        && mac[1] == 0xff
        && mac[2] == 0xff
        && mac[3] == 0xff)
    {
        g_testResult = MF_ERR_NO_MAC;
        g_sendCheckResult = true;
        return;
    }
    if(PHYInitError())
    {
        g_testResult = MF_ERR_PHY_NOT_FOUND;
        g_sendCheckResult = true;
        return;
    }
    
    NetBuildStart();
    NetbuildSearchDevice("ZCTEST", mfSearchResultHandle);
    SysTimerSet(searchDevice, 2000, 0, NULL);
   
}

static void mfFrameHandle(MFFrame_t *frame)
{
    uint8_t data[64];
    MFRespInfo_t *respInfo;
    MFWriteInfo_t *writeInfo;
    
    SysLog("type = %d", frame->type)
    switch(frame->type)
    {
        case MF_TYPE_COMM_TEST:
            mfFrameSend(MF_TYPE_COMM_TEST, NULL, 0);
            break;
        case MF_TYPE_WRITE_INFO:
            writeInfo = (MFWriteInfo_t *)frame->data;
            if(writeInfo->macEid == INFO_EID_MAC && writeInfo->macLen == PHY_MAC_LEN)
            {
                SysSetMacAddr(writeInfo->mac);
                mfFrameSend(MF_TYPE_WRITE_INFO, (uint8_t *)writeInfo, sizeof(MFWriteInfo_t));
            }
            break;
        case MF_TYPE_READ_INFO:
            respInfo = (MFRespInfo_t *)data;
            respInfo->verEid = INFO_EID_VER;
            respInfo->verLen = 4;
            memcpy(respInfo->version, SysGetVersion(), 4);
            respInfo->macEid = INFO_EID_MAC;
            respInfo->macLen = PHY_MAC_LEN;
            SysGetMacAddr(respInfo->mac);
            mfFrameSend(MF_TYPE_READ_INFO, data, sizeof(MFRespInfo_t));
            break;
        case MF_TYPE_START_CHECK:
            mfFrameSend(MF_TYPE_START_CHECK, NULL, 0);
            mfStartSelfCheck(data[0]);
            break;
        case MF_TYPE_CHECK_RESULT:
            g_sendCheckResult = false;
            g_testResult = 0;
            break;
        case MF_TYPE_UNLOCK:
            mfFrameSend(MF_TYPE_UNLOCK, NULL, 0);
            break;
        default:
            break;
    }
}

void MFRecvByte(uint8_t byte)
{
    if(!g_start)
    {
        return;
    }
    
    static uint8_t frameLength = 0;
    MFFrame_t *frame;
    
    g_frameBuf[g_count++] = byte;

    if(g_count == 1)
    {
        if(byte != MF_FRAME_HEAD_FLAG)
        {
            g_count = 0;
        }
    }
    else if(g_count == 2)
    {
        frameLength = byte;
        if(byte > MF_FRAME_BUFF_LEN - 2)
        {
            g_count = 0;
            frameLength = 0;
        }
    }
    else if(g_count == frameLength + 2)
    {
        frame = (MFFrame_t *)g_frameBuf;
        if(checkSum(&g_frameBuf[3], frameLength - 1) == frame->crc)
        {
            mfFrameHandle(frame);
        }
        g_count = 0;
        frameLength = 0;
    }
}

void MFStop(void)
{
    SysLog("");
    g_start = false;
    if(g_frameBuf != NULL)
    {
        free(g_frameBuf);
        g_frameBuf = NULL;
    }
    g_sendDataFunc = NULL;
}

void MFStart(MFSendDataFunc_t sendFunc)
{
    SysLog("");
    g_sendDataFunc = sendFunc;
    if(g_frameBuf == NULL)
    {
        g_frameBuf = (uint8_t *)malloc(MF_FRAME_BUFF_LEN);
    }
    g_count = 0;
    g_start = true;
}

void MFPoll(void)
{
    mfSendTestResult();
}

void MFInitialize(void)
{
}

