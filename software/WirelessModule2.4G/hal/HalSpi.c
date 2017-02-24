#include "HalSpi.h"
#include "HalWait.h"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_spi.h"


void HalSpiInitialize(void)
{
    SPI_InitTypeDef   SPI_InitStruct;
    
    GPIO_InitTypeDef  GPIO_InitStructure;
    /* < Configure SD_SPI pins: SCK MOSI*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*< Configure SD_SPI pins: MOSI */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*< Configure SD_SPI pins: MISO */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_PinAFConfig(GPIOA, GPIO_Pin_5, GPIO_AF_0);
    GPIO_PinAFConfig(GPIOA, GPIO_Pin_6, GPIO_AF_0);
    GPIO_PinAFConfig(GPIOA, GPIO_Pin_7, GPIO_AF_0);

    /* < Configure SD_SPI pins: CS 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_SET);
    */
    
    /*!< SD_SPI Config */
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;//SPI_BaudRatePrescaler_32;//chg cbl 0310
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStruct.SPI_CRCPolynomial = 7;
    
    SPI_Init(SPI1, &SPI_InitStruct);
    SPI_RxFIFOThresholdConfig(SPI1, SPI_RxFIFOThreshold_QF);
    SPI_Cmd(SPI1, ENABLE); /*!< SD_SPI enable */

}

void HalSpiReinit(void)
{
    SPI_Cmd(SPI1, DISABLE);
    SPI_I2S_DeInit(SPI1);
    HalSpiInitialize();
}

void HalSpiPoll(void)
{
    
}

#define MAX_RF_WAIT       20    //ms  
unsigned char HalSpiReadWrite(unsigned char data)
{
	/* Loop while DR register in not emplty */
	while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE) == RESET);
//	hal_wait_ms_cond(MAX_RF_WAIT, !(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE) == RESET) );
	/* Send byte through the SPI1 peripheral */
	SPI_SendData8(SPI1, data);
	/* Wait to receive a byte */
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	//while((SPI1->SR & SPI_I2S_FLAG_BSY) == RESET);
//	hal_wait_ms_cond(MAX_RF_WAIT, !(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == RESET) );
	/* Return the byte read from the SPI bus */
	return SPI_ReceiveData8(SPI1);
}
