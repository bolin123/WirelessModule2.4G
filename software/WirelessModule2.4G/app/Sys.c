#include "Sys.h"
#include "SysTimer.h"
#include "WirelessModule.h"
#include "Manufacture.h"
#include "HalFlash.h"
#if SYS_TEST_WM
#include "testWM.h"
#else
#include "MProto.h"
#endif

static uint8_t g_sysVersion[4] = {1, 0, 1, 1}; //系统版本号
//xorshift随机数算法
static uint32_t x = 123456789UL, y = 567819012UL, z = 321456780UL, w = 1234UL;

static uint32_t xorshift128(void) 
{
    uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

bool SysDataSendListEmpty(void)
{
#if SYS_TEST_WM
    return NetSendListEmpty();
#else
    return (NetSendListEmpty() && MProtoSendListEmpty());
#endif
}

bool SysGotDeviceInfo(void)
{
#if SYS_TEST_WM
    return true;
#else
    return MProtoGotDeviceInfo();
#endif
}

void SysUartRecvHandle(HalUart_t port, uint8_t byte)
{
    if(port == SYS_UART_COMM_PORT)
    {
        MFRecvByte(byte);
#if !SYS_TEST_WM
        MProtoRecvByte(byte);
#endif
    }
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
    SysLog("%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3]);
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
    MFInitialize();
    WMInitialize();
    
#if SYS_TEST_WM
    testWMInit();
#else
    MProtoInitialize();
#endif
    SysPrintf("===============================\n");
    SysPrintf("Firmware version:%d.%d.%d.%d\n", g_sysVersion[0], g_sysVersion[1], 
                                                g_sysVersion[2], g_sysVersion[3]);
    SysPrintf("Build time:%s\n", __DATE__" "__TIME__);
    SysPrintf("================================\n");

}

void SysPoll(void)
{
#if SYS_TEST_WM
    testWMPoll();
#else
    MProtoPoll();
#endif
    WMPoll(); 
    MFPoll();
    HalPoll();
    SysTimerPoll();
}

