#include "HalCommon.h"
#include "HalClk.h"
#include "HalWdg.h"
#include "HalGPIO.h"
#include "HalTimer.h"
#include "HalUart.h"
#include "HalSpi.h"
#include "HalFlash.h"
#include "HalWait.h"

#define HAL_STATUS_LED_PIN  0x11 //PB1

static uint32_t volatile g_sysTimerCount;
static bool g_intEnable = true;

//led blink args
static uint8_t g_blinkCount = 0;
static SysTime_t g_lastBlinkTime;
static uint8_t g_statusCount = 0;

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

/*
* δ������˸3��
* ��������˸2��
* ��������˸1��
*/
void HalStatusLedSet(uint8_t blinkCount)
{
    g_blinkCount = blinkCount * 2;
    g_statusCount = 0;
    g_lastBlinkTime = SysTime();
}

static void HalStatusLedPoll(void)
{
    if(g_statusCount < g_blinkCount)
    {
        if(SysTimeHasPast(g_lastBlinkTime, 100))
        {
            HalGPIOSet(HAL_STATUS_LED_PIN, !HalGPIOGet(HAL_STATUS_LED_PIN));
            g_lastBlinkTime = SysTime();
            g_statusCount++;
        }
    }
    else
    {
        HalGPIOSet(HAL_STATUS_LED_PIN, HAL_GPIO_LEVEL_HIGH);
        if(SysTimeHasPast(g_lastBlinkTime, 2000))
        {
            g_statusCount = 0;
        }
    }
}

static void HalStatusLedInit(void)
{
    HalGPIOInit(HAL_STATUS_LED_PIN, HAL_GPIO_DIR_OUT);
    HalGPIOSet(HAL_STATUS_LED_PIN, HAL_GPIO_LEVEL_HIGH);
}

void HardFault_Handler(void)
{
    SysLog("");
    HalReboot();
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
    HalStatusLedInit();
    debugUartInit();
}

ROM_FUNC void HalPoll(void)
{
    HalGPIOPoll();
    HalSpiPoll();
    HalFlashPoll();
    HalUartPoll();
    //HalIwdtFeed();
    HalStatusLedPoll();
}

