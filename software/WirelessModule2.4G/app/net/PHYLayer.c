#include "PHYLayer.h"
#include "HalWait.h"
#include "Sys.h"
#include "VTList.h"

#define PHY_FRAME_PREAMBLE 0xa5

#define PHY_BROADCAST_ADDR 0xff
#define PHY_RESEND_MAX_TIME 3
#define PHY_RESEND_TIME 200

#define PHY_FRAME_HEAD_LEN sizeof(PHYFrame_t)

typedef struct
{
    uint8_t dstMac[PHY_MAC_LEN];
    uint8_t srcMac[PHY_MAC_LEN];
}AddrInfoEstNetwork_t;

typedef struct
{
    uint8_t segAddr[PHY_MAC_LEN];
    uint8_t dstAddr;
    uint8_t srcAddr;
    uint8_t reserve[2];
}AddrInfoNormal_t;

typedef struct
{
    uint8_t preamble;
    uint8_t type;
    uint8_t addrInfo[8];
    uint8_t pid;
    uint8_t dataLen;
    uint8_t data[];
}PHYFrame_t;

typedef struct PHYSendList_st
{
    uint8_t pid;
    uint8_t count;
    uint8_t dlen;
    uint8_t *data;
    uint32_t lastTime;
    VTLIST_ENTRY(struct PHYSendList_st);
}PHYSendList_t;

static const uint8_t BRAODCASTMAC[PHY_MAC_LEN] = {0xff, 0xff, 0xff, 0xff};
static uint8_t g_phyMacAddr[PHY_MAC_LEN];
static uint16_t g_recvPid = 0xffff;
static uint8_t g_sendPid = 0;
static uint8_t g_segAddr[PHY_MAC_LEN];
static uint8_t g_myNetAddr = 0xff;
static PHYSendList_t g_phySendList;

static PHYOptHandle_cb g_optHandle = NULL;
static PHYEstnetHandle_cb g_estnetHandle = NULL;
static PHYHeartbeatHandle_cb g_heartbeatHandle = NULL;

static uint8_t g_phyRfChannel = 0;
static uint8_t g_recvBuff[255];
static uint8_t g_recvCount = 0;
static uint16_t g_resendOffsetTime = 0;

void PHYSetMac(const uint8_t mac[PHY_MAC_LEN])
{
    HalPrintf("Set Mac:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3]);
    memcpy(g_phyMacAddr, mac, PHY_MAC_LEN);
}

uint8_t *PHYGetMac(uint8_t *mac)
{
    if(mac != NULL)
    {
        memcpy(mac, g_phyMacAddr, PHY_MAC_LEN);
    }
    return g_phyMacAddr;
}

void PHYSetNetAddr(uint8_t addr)
{
    HalPrintf("Set Net Addr:%d\n", addr);
    g_myNetAddr = addr;
}

void PHYSetSegAddr(const uint8_t segAddr[PHY_MAC_LEN])
{
    HalPrintf("Set segAddr:%02x:%02x:%02x:%02x\n", segAddr[0], segAddr[1], segAddr[2], segAddr[3]);
    memcpy(g_segAddr, segAddr, PHY_MAC_LEN);
}

static uint8_t sumCheck(const uint8_t *data, uint8_t len)
{
    uint8_t i, sum = 0;

    for(i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return sum;
}

static void lowLevelDataSend(const uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint8_t count = len / RX_PLOAD_WIDTH;
    uint8_t last = len % RX_PLOAD_WIDTH;
    uint8_t buff[RX_PLOAD_WIDTH];

    HalPrintf("Send:");
    for(i = 0; i < len; i++)
    {
        HalPrintf("%02x ", data[i]);
    }
    HalPrintf("\n");
    
    NRF24L01ModeSet(NRF24L01SendMode, g_phyRfChannel);
    for(i = 0; i < count; i++)
    {
        NRF24L01TxPacket(&data[i * RX_PLOAD_WIDTH], RX_PLOAD_WIDTH);
        hal_wait_us(50);
    }

    if(last)
    {
        memcpy(buff, &data[len - last], last);
        NRF24L01TxPacket(buff, RX_PLOAD_WIDTH);
        hal_wait_us(50);
    }
    NRF24L01ModeSet(NRF24L01RecvMode, g_phyRfChannel);
}

static void phyDataSend(uint8_t pid, uint8_t *data, uint8_t len)
{
    PHYSendList_t *packet = (PHYSendList_t *)malloc(sizeof(PHYSendList_t));

    packet->pid = pid;
    packet->count = 0;
    packet->dlen = len;
    packet->lastTime = 0;
    packet->data = (uint8_t *)malloc(len);
    memcpy(packet->data, data, len);
    VTListAdd(&g_phySendList, packet);
    //lowLevelDataSend(data, len);
}

static void phyDataListHandle(void)
{
    PHYSendList_t *packet = VTListFirst(&g_phySendList);

    if(packet != NULL)
    {
        if(SysTimeHasPast(packet->lastTime, PHY_RESEND_TIME + g_resendOffsetTime))
        {
            lowLevelDataSend(packet->data, packet->dlen);
            packet->count++;
            packet->lastTime = SysTime();
            SysRandomSeed(SysTime());
            g_resendOffsetTime = SysRandom() % 100;
            //SysPrintf("retry:%d, time:%d\n", packet->count, SysTime());
            if(packet->count >= PHY_RESEND_MAX_TIME)
            {
                VTListDel(packet);
                free(packet->data);
                free(packet);
            }
        }
    }
}

static void phyAck(uint8_t dstAddr, uint8_t pid)
{
    uint8_t buff[32];
    PHYFrame_t *frame = (PHYFrame_t *)buff;
    AddrInfoNormal_t *addr = (AddrInfoNormal_t *)frame->addrInfo;

    HalPrintf("ack pid:%d\n", pid);
    frame->preamble = PHY_FRAME_PREAMBLE;
    frame->type = PHY_FRAME_TYPE_ACK;
    memcpy(addr->segAddr, g_segAddr, PHY_MAC_LEN);
    addr->dstAddr = dstAddr;
    addr->srcAddr = g_myNetAddr;
    frame->pid = pid;
    frame->dataLen = 0;

    buff[PHY_FRAME_HEAD_LEN] = sumCheck(buff, PHY_FRAME_HEAD_LEN);

    lowLevelDataSend(buff, PHY_FRAME_HEAD_LEN + 1);
}

#if 0
static void phyReinit(void)
{
    SysLog("");
    memset(g_recvBuff, 0, sizeof(g_recvBuff));
    g_recvCount = 0;
    NRF24l01Shutdown();
    HalSpiReinit();
}
#endif

void PHYSwitchChannel(uint8_t ch)
{
    g_phyRfChannel = ch;
    NRF24L01ModeSet(NRF24L01RecvMode, ch);
    SysLog("channel:%d", ch);
}

void PHYPacketRecvHandle(uint8_t *data, uint8_t len)
{
    PHYFrame_t *frame = (PHYFrame_t *)data;
    AddrInfoEstNetwork_t *estNwkAddrInfo = NULL;
    AddrInfoNormal_t *normalAddrInfo = NULL;
    PHYSendList_t *sendPacket;
    bool isBroadcast = false;
    uint8_t ackAddr, ackPid;
    uint8_t frameType = frame->type & 0x7f;
    uint8_t i;
    static uint8_t lastDevAddr = 0xff;
    
    if(frameType == PHY_FRAME_TYPE_EST_NETWORK)
    {
        estNwkAddrInfo = (AddrInfoEstNetwork_t *)frame->addrInfo;
        if(memcmp(estNwkAddrInfo->dstMac, g_phyMacAddr, PHY_MAC_LEN) == 0)
        {
            isBroadcast = false;
            ackAddr = 0; //host net address always 0
            ackPid = frame->pid;
            memcpy(g_segAddr, estNwkAddrInfo->srcMac, PHY_MAC_LEN); //临时设置段地址，用于应答
        }
        else if(memcmp(estNwkAddrInfo->dstMac, BRAODCASTMAC, PHY_MAC_LEN) == 0)
        {
            isBroadcast = true;
        }
        else
        {
            return;
        }
    }
    else
    {
        normalAddrInfo = (AddrInfoNormal_t *)frame->addrInfo;
        if(memcmp(normalAddrInfo->segAddr, g_segAddr, PHY_MAC_LEN) == 0) //segment address
        {
            if(normalAddrInfo->dstAddr == g_myNetAddr) // network address
            {
                isBroadcast = false;
                ackAddr = normalAddrInfo->srcAddr;
                ackPid = frame->pid;
            }
            else if(normalAddrInfo->dstAddr == PHY_BROADCAST_ADDR)
            {
                isBroadcast = true;
            }
            else
            {
                return;
            }
        }
        else
        {
            return;
        }
    }
    
#if 1
    HalPrintf("Recv:");
    for(i = 0; i < len; i++)
    {
        HalPrintf("%02x ", data[i]);
    }
    HalPrintf("\n");
#endif

    if(!isBroadcast && frameType != PHY_FRAME_TYPE_ACK && frameType != PHY_FRAME_TYPE_HEARTBEAT) //ack
    {
        phyAck(ackAddr, ackPid);
    }
#if 1
    if(frameType != PHY_FRAME_TYPE_EST_NETWORK && frameType != PHY_FRAME_TYPE_ACK)
    {
        if(g_recvPid == frame->pid && lastDevAddr == normalAddrInfo->srcAddr)
        {
            HalPrintf("repeat message, ignore %d\n", g_recvPid);
            return;
        }
        
        lastDevAddr = normalAddrInfo->srcAddr;
    }
    g_recvPid = frame->pid;
#endif
    switch(frameType)
    {
        case PHY_FRAME_TYPE_OPERATION:
            if(g_optHandle)
            {
                g_optHandle(normalAddrInfo->srcAddr, frame->type & 0x80, isBroadcast, frame->data, frame->dataLen);
            }
            break;
        case PHY_FRAME_TYPE_EST_NETWORK:
            if(g_estnetHandle)
            {
                g_estnetHandle(estNwkAddrInfo->srcMac, isBroadcast, frame->data, frame->dataLen);
            }
            break;
        case PHY_FRAME_TYPE_ACK:
            VTListForeach(&g_phySendList, sendPacket)
            {
                if(sendPacket->pid == frame->pid)
                {
                    HalPrintf("recv pid:%d ack\n", frame->pid);
                    VTListDel(sendPacket);
                    free(sendPacket->data);
                    free(sendPacket);
                    sendPacket = NULL;
                    break;
                }
            }
            break;
        case PHY_FRAME_TYPE_HEARTBEAT:
            if(g_heartbeatHandle)
            {
                g_heartbeatHandle(normalAddrInfo->srcAddr, frame->type & 0x80);
            }
            break;
        default:
            break;
    }

}

static void phyRecvDataHandle(void)
{
    uint8_t i;
    uint8_t buff[RX_PLOAD_WIDTH] = {0};
    static SysTime_t lastRecvTime;
    static uint8_t buffLength = 0;
    
    if(NRF24L01DataRecved())
    {
        if(NRF24L01RxPacket(buff) == 0)
        {
            if(SysTimeHasPast(lastRecvTime, 100)) //两个帧之间>100ms，重新开始计数
            {
                g_recvCount = 0;
            }
            lastRecvTime = SysTime();
            for(i = 0; i < RX_PLOAD_WIDTH; i++)
            {
                g_recvBuff[g_recvCount++] = buff[i];

                if(g_recvCount == 1)
                {
                    if(buff[i] != PHY_FRAME_PREAMBLE)
                    {
                        g_recvCount = 0;
                    }
                }
                else if(g_recvCount == 12)
                {
                    buffLength = buff[i];
                }
                else if(g_recvCount == sizeof(PHYFrame_t) + buffLength + 1)//head + contents + crc
                {
                    //crc check 
                    PHYPacketRecvHandle(g_recvBuff, g_recvCount);
                    g_recvCount = 0;
                    buffLength = 0;
                    break;
                }
            }
        }
    }
}

/*心跳帧发送函数
* dstAddr:目标地址, isSleep:本设备是否休眠设备
*/
void PHYHeartbeatSend(uint8_t dstAddr, bool isSleep)
{
    uint8_t buff[64];
    AddrInfoNormal_t *addrInfo;
    PHYFrame_t *frame = (PHYFrame_t *)buff;

    frame->preamble = PHY_FRAME_PREAMBLE;
    frame->type = (uint8_t)PHY_FRAME_TYPE_HEARTBEAT | (isSleep ? 0x80 : 0x00);
    addrInfo = (AddrInfoNormal_t *)frame->addrInfo;
    memcpy(addrInfo->segAddr, g_segAddr, PHY_MAC_LEN);
    addrInfo->dstAddr = dstAddr;
    addrInfo->srcAddr = g_myNetAddr;
    frame->pid = g_sendPid++;
    frame->dataLen = 0;
    buff[PHY_FRAME_HEAD_LEN] = sumCheck(buff, PHY_FRAME_HEAD_LEN);
    lowLevelDataSend(buff, PHY_FRAME_HEAD_LEN + 1);
}

/*业务帧发送函数
* dstAddr:目标地址, isSleep:本设备是否休眠设备,
* data:数据内容, len:数据长度
*/
void PHYOperatePacketSend(uint8_t dstAddr, bool isSleep, const uint8_t *data, uint8_t len, bool directly)
{
    uint8_t buff[255], dlen;
    AddrInfoNormal_t *addrInfo;
    PHYFrame_t *frame = (PHYFrame_t *)buff;

    frame->preamble = PHY_FRAME_PREAMBLE;
    frame->type = (uint8_t)PHY_FRAME_TYPE_OPERATION | (isSleep ? 0x80 : 0x00);
    addrInfo = (AddrInfoNormal_t *)frame->addrInfo;
    memcpy(addrInfo->segAddr, g_segAddr, PHY_MAC_LEN);
    addrInfo->dstAddr = dstAddr;
    addrInfo->srcAddr = g_myNetAddr;
    frame->pid = g_sendPid++;
    frame->dataLen = len;
    memcpy(frame->data, data, len);

    dlen = PHY_FRAME_HEAD_LEN + len;
    buff[dlen] = sumCheck(buff, dlen);
    if(directly || dstAddr == PHY_BROADCAST_ADDR)
    {
        lowLevelDataSend(buff, dlen + 1);
    }
    else
    {
        phyDataSend(frame->pid, buff, dlen + 1);
    }
}

/*组网帧发送函数
* dstMac:目标Mac地址, directly:是否需要重发
* data:数据内容, len:数据长度
*/
void PHYEstnetPacketSend(const uint8_t dstMac[PHY_MAC_LEN], const uint8_t *data, uint8_t len, bool directly)
{
    uint8_t buff[255], dlen;
    AddrInfoEstNetwork_t *addInfo;
    PHYFrame_t *frame = (PHYFrame_t *)buff;

    frame->preamble = PHY_FRAME_PREAMBLE;
    frame->type = PHY_FRAME_TYPE_EST_NETWORK;
    addInfo = (AddrInfoEstNetwork_t *)frame->addrInfo;
    memcpy(addInfo->dstMac, dstMac, PHY_MAC_LEN);
    memcpy(addInfo->srcMac, g_phyMacAddr, PHY_MAC_LEN);
    frame->pid = g_sendPid++;
    frame->dataLen = len;
    memcpy(frame->data, data, len);

    dlen = PHY_FRAME_HEAD_LEN + len;
    buff[dlen] = sumCheck(buff, dlen);
    if(directly || memcmp(dstMac, BRAODCASTMAC, PHY_MAC_LEN) == 0)
    {
        lowLevelDataSend(buff, dlen + 1);
    }
    else
    {
        phyDataSend(frame->pid, buff, dlen + 1);
    }
}

void PHYPacketHandleCallbackRegiste(PHYOptHandle_cb optCb, 
                                             PHYEstnetHandle_cb estnetCb, 
                                             PHYHeartbeatHandle_cb hbCb)
{
    g_optHandle = optCb;
    g_estnetHandle = estnetCb;
    g_heartbeatHandle = hbCb;
}

void PHYSetSleepMode(bool sleep)
{
    NRF24L01SetSleepMode(sleep);
}

void PHYInitialize(void)
{
    VTListInit(&g_phySendList);
    NRF24L01Initialize();

    if(!NRF24L01Check())
    {
        SysLog("PHY device not found!!!");
    }
    SysLog("PHY device init ok!");
}

void PHYPoll(void)
{
    NRF24L01Poll();
    phyDataListHandle();
    phyRecvDataHandle();
}

