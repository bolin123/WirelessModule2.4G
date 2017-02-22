#include "HalCommon.h"
#include "HalClk.h"
#include "HalWdt.h"
#include "HalGPIO.h"
#include "HalTimer.h"
#include "HalUart.h"
#include "HalSpi.h"
#include "HalFlash.h"
#include "HalWait.h"

static uint32_t volatile g_sysTimerCount;
static bool g_intEnable = true;

//redirect "printf()"
int fputc(int ch, FILE *f)
{
	HalUartWrite(HAL_UART_0, (const uint8_t *)&ch, 1);
	return ch;
}

static void debugUartInit(void)
{
    HalUartConfig_t uartConfig;
    uartConfig.baudRate = 115200;
    uartConfig.parity = PARITY_NONE;
    uartConfig.recvHandler = NULL;
    HalUartInit(HAL_UART_0, &uartConfig);
}

bool HalInterruptsGetEnable(void)
{
    return g_intEnable;
}

void HalInterruptsSetEnable(bool enable)
{
    g_intEnable = enable;
    if(enable)
    {
        __enable_irq();
    }
    else
    {
        __disable_irq();
    }
}

void HalReboot(void)
{
    static bool reboot = false;
    if(!reboot)
    {
        SysLog("");
        reboot = true;
		//__set_FAULTMASK(1);
		NVIC_SystemReset();
    }
}

uint32_t HalRunningTime(void)
{
    return g_sysTimerCount;
}


void HalTimerPassMs(void)
{
	g_sysTimerCount++;
}

void HalInitialize(void)
{
	HalClkInit();
    HalTimerInitialize();
    HalGPIOInitialize();
    HalUartInitialize();
    HalSpiInitialize();
    HalFlashInitialize();
    //HalIwdtInitialize();
    debugUartInit();
}

ROM_FUNC void HalPoll(void)
{
    HalGPIOPoll();
    HalSpiPoll();
    HalFlashPoll();
    HalUartPoll();
    //HalIwdtFeed();
}

