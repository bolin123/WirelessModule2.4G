#include "Sys.h"
#include "SysTimer.h"
#include "WirelessModule.h"

//xorshiftËæ»úÊýËã·¨
uint32_t x = 123456789UL, y = 567819012UL, z = 321456780UL, w = 1234UL;
static uint32_t xorshift128(void) {
    uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

uint32_t SysRandom(void)
{
    return xorshift128();
}

uint8_t *SysGetMacAddr(void)
{
    return NULL;
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

void SysInitialize(void)
{
    HalInitialize();
    SysTimerInitialize();
    WMInitialize();
}

void SysPoll(void)
{
    WMPoll();
    HalPoll();
    SysTimerPoll();
}
