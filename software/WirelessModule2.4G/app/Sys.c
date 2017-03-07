#include "Sys.h"
#include "SysTimer.h"
#include "WirelessModule.h"
#include "MProto.h"
#include "Manufacture.h"
#include "HalFlash.h"
//#include "testWM.h"

static uint8_t g_sysVersion[4] = {1, 0, 0, 0}; //系统版本号
//xorshift随机数算法
static uint32_t x = 123456789UL, y = 567819012UL, z = 321456780UL, w = 1234UL;

static uint32_t xorshift128(void) 
{
    uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

void SysUartRecvHandle(uint8_t byte)
{
    if(!MProtoGotDeviceInfo())
    {
        MFRecvByte(byte);
    }
    MProtoRecvByte(byte);
}

uint8_t *SysGetVersion(void)
{
    return g_sysVersion;
}

uint32_t SysRandom(void)
{
    return xorshift128();
}

void SysSetMacAddr(uint8_t *mac)
{
    HalFlashErase(SYS_DEVICE_MAC_ADDR);
    HalFlashWrite(SYS_DEVICE_MAC_ADDR, mac, PHY_MAC_LEN);
}

uint8_t *SysGetMacAddr(uint8_t *mac)
{
    HalFlashRead(SYS_DEVICE_MAC_ADDR, mac, PHY_MAC_LEN);
    return mac;
}

const uint8_t *SysUidToMac(uint32_t uid)
{
    static uint8_t mac[PHY_MAC_LEN];

    mac[0] = (uint8_t)((uid >> 24) & 0xff);
    mac[1] = (uint8_t)((uid >> 16) & 0xff);
    mac[2] = (uint8_t)((uid >> 8) & 0xff);
    mac[3] = (uint8_t)(uid & 0xff);

    return mac;
}

uint32_t SysMacToUid(const uint8_t *mac)
{
    uint8_t i;
    uint32_t num = 0;

    for(i = 0; i < PHY_MAC_LEN; i++)
    {
        num = (num << 8) + mac[i];
    }
    return num;
}


void SysRandomSeed(uint32_t seed)
{
    x = seed;
    y = SysRandom();
    z = SysRandom();
    w = SysRandom();
}

void SysReboot(void)
{
    HalReboot();
}

void SysInitialize(void)
{
    HalInitialize();
    SysTimerInitialize();
    MProtoInitialize();
    MFInitialize();
    //testWMInit();
}

void SysPoll(void)
{
    //testWMPoll();
    MProtoPoll();
    MFPoll();
    HalPoll();
    SysTimerPoll();
}
