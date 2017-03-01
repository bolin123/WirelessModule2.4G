#include "Sys.h"
#if defined(PC_ENV)
#include <Windows.h>
#endif
//#include "testWM.h"

#if defined(PC_ENV)
#define SLEEP_MS() Sleep(1)

static uint32_t g_sysTimeCount = 0;

uint32_t HalGetTimeCount(void)
{
    return g_sysTimeCount;
}
#endif

int main(void)
{
    SysInitialize();
    //testWMInit();
    while(1)
    {
        //testWMPoll();
        SysPoll();
        #if defined(PC_ENV)
        SLEEP_MS();
        g_sysTimeCount++;
        #endif
    }
}
