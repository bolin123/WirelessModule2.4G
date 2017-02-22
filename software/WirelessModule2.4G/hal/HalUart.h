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

//���ݽ��ջص�
typedef void (*HalUartRecvHandler_t)(HalUart_t uart, const uint8_t *data, uint16_t len);

//�������ò���
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
 *  ��������
 *  @param cfg ���ò���
 */
void HalUartInit(HalUart_t uart, const HalUartConfig_t *cfg);

/**
 *  ����д��
 *  @param uart  ָ������
 *  @param data ����
 *  @param len ���ݳ���
 *  @return �ɹ��������ݳ���ʧ�ܷ���0
 */
uint16_t HalUartWrite(HalUart_t uart, const void *data, uint16_t len);

#endif // HAL_UART_H





