#ifndef NRF24L01_H
#define NRF24L01_H

#include "HalCtype.h"

#define TX_ADR_WIDTH    4   //5¡Á??¨²¦Ì?¦Ì??¡¤?¨ª?¨¨
#define RX_ADR_WIDTH    4   //5¡Á??¨²¦Ì?¦Ì??¡¤?¨ª?¨¨
#define TX_PLOAD_WIDTH  32  //20¡Á??¨²¦Ì?¨®??¡ì¨ºy?Y?¨ª?¨¨
#define RX_PLOAD_WIDTH  32  //20¡Á??¨²¦Ì?¨®??¡ì¨ºy?Y?¨ª?¨¨

typedef enum
{
    NRF24L01RecvMode = 0,
    NRF24L01SendMode,
}NRF24L01Mode_t;

void NRF24L01Initialize(void);
void NRF24L01Poll(void);
bool NRF24L01Check(void);
bool NRF24L01DataRecved(void);
uint8_t NRF24L01TxPacket(const uint8_t *txbuf, uint8_t len);
uint8_t NRF24L01RxPacket(uint8_t *rxbuf);
void NRF24L01ModeSet(NRF24L01Mode_t mode, uint8_t chn);
uint8_t NRF24L01GetStatus(void);
void NRF24l01Shutdown(void);
void NRF24L01Sleep(bool sleep);

#endif

