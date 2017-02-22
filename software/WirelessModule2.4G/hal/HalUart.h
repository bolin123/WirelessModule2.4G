#ifndef HAL_UART_H
#define HAL_UART_H

#include "HalCtype.h"

//Uart
typedef enum
{
    HAL_UART_0, //sim800c
    HAL_UART_1, //com
    HAL_UART_2, //debug
    HAL_UART_COUNT,
    HAL_UART_INVALID = 0xff,
}HalUart_t;

typedef enum
{
    PARITY_NONE = 0,
    PARITY_EVEN,
    PARITY_ODD,
}HalUartParity_t;

//数据接收回调
typedef void (*HalUartRecvHandler_t)(HalUart_t uart, const uint8_t *data, uint16_t len);

//串口配置参数
typedef struct
{
    uint32_t baudRate;
    HalUartParity_t parity;
    HalUartRecvHandler_t recvHandler;
}HalUartConfig_t;

void HalUartInitialize(void);
void HalUartPoll(void);

void HalUartRecvData(HalUart_t uart, const uint8_t *data, uint16_t len);

/**
 *  串口配置
 *  @param cfg 配置参数
 */
void HalUartInit(HalUart_t uart, const HalUartConfig_t *cfg);

/**
 *  串口写入
 *  @param uart  指定串口
 *  @param data 数据
 *  @param len 数据长度
 *  @return 成功返回数据长度失败返回0
 */
uint16_t HalUartWrite(HalUart_t uart, const void *data, uint16_t len);

#endif // HAL_UART_H





