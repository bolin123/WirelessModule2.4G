#include "HalUart.h"
#include "HalGPIO.h"
#include "HalCommon.h"

#define UART_MAX_NUM  2
#define HAL_USART_TX_GPIO        0x07

typedef void (*HalUartDmaRecvIncreaseCb_t)(HalUart_t uart, uint16_t len);

typedef enum{
    UART_RECV_INT = 0,
    UART_RECV_DMA,
}UartRecvMode_t;

typedef struct{
    uint8_t irq;
    uint8_t priority;
    uint16_t ioRX;
    uint16_t rxPinSource;
    uint16_t ioTX;
    uint16_t txPinSource;
    UartRecvMode_t mode;
    uint32_t baudRate;
    USART_TypeDef* uartNO;
    GPIO_TypeDef* gpioX;
    HalUartRecvHandler_t cb;
    HalUartDmaRecvIncreaseCb_t incHandle;
}halUartConfig_t;

//static halUartConfig_t g_uartConfig[UART_MAX_NUM];
//static uint8_t *g_dmaBuffer;
//static uint16_t g_dmaBufferSize = 0;

static void NVIC_USART_Configuration(uint8_t irqChannel, uint8_t priority)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    
    NVIC_InitStructure.NVIC_IRQChannel = irqChannel;
    NVIC_InitStructure.NVIC_IRQChannelPriority = priority;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

}

#if 0
static void usartDmaRecvInit(USART_TypeDef* uartNO, uint8_t *buff, uint16_t len)
{
    DMA_InitTypeDef DMA_InitStructure;
    DMA_Channel_TypeDef *dmaCh;
    if(uartNO == USART1)
    {
        dmaCh = DMA1_Channel3;
    }else if(uartNO == USART2){
        dmaCh = DMA1_Channel5;
    }

    USART_DMACmd(uartNO, USART_DMAReq_Rx, DISABLE);
    DMA_Cmd(dmaCh, DISABLE);

    DMA_DeInit(dmaCh);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&uartNO->RDR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)buff;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = len;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(dmaCh, &DMA_InitStructure);

    USART_DMACmd(uartNO, USART_DMAReq_Rx, ENABLE); 
    
    DMA_Cmd(dmaCh, ENABLE); 
//    g_dmaBufferLen = len;
}
#endif

static void usartConfiguration(const HalUartConfig_t *cfg, USART_TypeDef* uartNO, UartRecvMode_t mode)
{
    USART_InitTypeDef USART_InitStructure;
    uint32_t wordLength, parity;

    USART_Cmd(uartNO, DISABLE);
    if(mode == UART_RECV_INT)
    {
        USART_ITConfig(uartNO, USART_IT_RXNE, DISABLE);
    }

    if(cfg->parity == PARITY_EVEN)
    {
        wordLength = USART_WordLength_9b;
        parity = USART_Parity_Even;
    }
    else if(cfg->parity == PARITY_ODD)
    {
        wordLength = USART_WordLength_9b;
        parity = USART_Parity_Odd;
    }
    else
    {
        wordLength = USART_WordLength_8b;
        parity = USART_Parity_No;
    }
    
    USART_InitStructure.USART_BaudRate            = cfg->baudRate;
    USART_InitStructure.USART_WordLength          = wordLength;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = parity;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(uartNO, &USART_InitStructure);

    USART_OverrunDetectionConfig(uartNO, USART_OVRDetection_Disable);
    if(mode == UART_RECV_INT)
    {
        USART_ClearITPendingBit(uartNO, USART_IT_RXNE);
        USART_ITConfig(uartNO, USART_IT_RXNE, ENABLE);
    }
    #if 0
    else if(mode == UART_RECV_DMA)
    {
        usartDmaRecvInit(uartNO, g_dmaBuffer, g_dmaBufferSize);
    }
    #endif
    
    USART_Cmd(uartNO, ENABLE);
}

/*
void HalUartDmaInit(uint8_t uart, uint8_t *buff, uint16_t len, void *cb)
{
    g_dmaBuffer = buff;
    g_dmaBufferSize = len;
    g_uartConfig[uart].incHandle = (HalUartDmaRecvIncreaseCb_t)cb;
}
*/

static void usartInit(halUartConfig_t *config)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    if(config->mode == UART_RECV_INT)
    {
        NVIC_USART_Configuration(config->irq, config->priority);
    }

    GPIO_PinAFConfig(config->gpioX, config->rxPinSource, GPIO_AF_1);
    GPIO_PinAFConfig(config->gpioX, config->txPinSource, GPIO_AF_1);
    
    GPIO_InitStructure.GPIO_Pin   = config->ioTX;  //TX
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(config->gpioX, &GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Pin   = config->ioRX;  //RX
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_Init(config->gpioX, &GPIO_InitStructure);
    
}


ROM_FUNC void HalUartInitialize(void)
{
    halUartConfig_t uart;
#if 1 
    //uart = &g_uartConfig[HAL_UART_0];
    //print uart port(日志打印串口)
    uart.baudRate = 115200;
    uart.cb       = NULL;
    uart.gpioX    = GPIOA;
    uart.ioRX     = GPIO_Pin_10;
    uart.rxPinSource = GPIO_PinSource10;
    uart.ioTX     = GPIO_Pin_9;
    uart.txPinSource = GPIO_PinSource9;
    uart.irq      = USART1_IRQn;
    uart.priority = HAL_UART_0;
    uart.uartNO   = USART1;
    uart.mode     = UART_RECV_INT;
    uart.incHandle = NULL;
    usartInit(&uart);
#endif 
    //uart = &g_uartConfig[HAL_UART_1];
    //communicate uart port(数据通信串口)
    uart.baudRate = 115200;
    uart.cb       = NULL;
    uart.gpioX    = GPIOA;
    uart.ioRX     = GPIO_Pin_3;
    uart.rxPinSource = GPIO_PinSource3;
    uart.ioTX     = GPIO_Pin_2;
    uart.txPinSource = GPIO_PinSource2;
    uart.irq      = USART2_IRQn;
    uart.priority = HAL_UART_1;
    uart.uartNO   = USART2;
    uart.mode     = UART_RECV_INT;
    uart.incHandle = NULL;
    usartInit(&uart);
}

ROM_FUNC void HalUartPoll(void)
{

}

extern void SysUartRecvHandle(uint8_t byte);

static void HalUartDataCallback(HalUart_t uart, uint8_t data)
{
    if(uart == HAL_UART_1)
    {
        SysUartRecvHandle(data);
    }
}

void HalUartInit(HalUart_t uart, const HalUartConfig_t *cfg)
{
    if(uart == HAL_UART_0)
    {
        usartConfiguration(cfg, USART1, UART_RECV_INT);
    }
    else if(uart == HAL_UART_1)
    {
        usartConfiguration(cfg, USART2, UART_RECV_INT);
    }
    else
    {
    }
}

ROM_FUNC void HalUartSetPrintUart(HalUart_t uart)
{

}


ROM_FUNC uint16_t HalUartWrite(HalUart_t uart, const void *data, uint16_t len)
{
    uint16_t i;
    uint8_t *dataByte = (uint8_t *)data;
    USART_TypeDef *uartType;

    if(uart == HAL_UART_0)
    {
        uartType = USART1;
    }
    else if(uart == HAL_UART_1)
    {
        uartType = USART2;
    }
    else
    {
        return 0;
    }

    for(i=0; i<len; i++)
    {
        USART_SendData(uartType, (uint16_t)dataByte[i]);
        while(USART_GetFlagStatus(uartType, USART_FLAG_TXE) == RESET); //wait for Transmit data register empty
    }
    USART_ClearITPendingBit(uartType, USART_IT_TC);
    return len;
}

//uart1 dma recevie without irq handle

void USART1_IRQHandler(void)
{
    uint16_t recvDate=0;    
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        //USART_ClearITPendingBit(USART3, USART_IT_RXNE);
        recvDate = USART_ReceiveData(USART1);
        USART_RequestCmd(USART1, USART_Request_RXFRQ, ENABLE);
        HalUartDataCallback(HAL_UART_0, (uint8_t)recvDate);
    }
    
}

void USART2_IRQHandler(void)
{
    uint16_t recvDate=0;    
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        //USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        recvDate = USART_ReceiveData(USART2);
        USART_RequestCmd(USART2, USART_Request_RXFRQ, ENABLE);
        HalUartDataCallback(HAL_UART_1, (uint8_t)recvDate);
    }
    
}


