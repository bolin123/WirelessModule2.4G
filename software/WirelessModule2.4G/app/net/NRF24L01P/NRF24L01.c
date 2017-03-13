#include "NRF24L01.h"
#include "HalGPIO.h"
#include "HalSpi.h"
#include "HalWait.h"
#include "HalCommon.h"
#include "stm32f0xx_syscfg.h"


#define NRF24L01_CE_PIN   0x10 //PB0
#define NRF24L01_CSN_PIN  0x04 //PA4
#define NRF24L01_IRQ_PIN  0x01 //PA1

#define NRF24L01_CSN_LOW() HalGPIOSet(NRF24L01_CSN_PIN, HAL_GPIO_LEVEL_LOW)
#define NRF24L01_CSN_HIGH() HalGPIOSet(NRF24L01_CSN_PIN, HAL_GPIO_LEVEL_HIGH)
#define NRF24L01_CE_LOW() HalGPIOSet(NRF24L01_CE_PIN, HAL_GPIO_LEVEL_LOW)
#define NRF24L01_CE_HIGH() HalGPIOSet(NRF24L01_CE_PIN, HAL_GPIO_LEVEL_HIGH)

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//NRF24L01??��??��2������?����?
#define READ_REG        0x00  //?��??????��??��,�̨�5???a??��??�¦�??��
#define WRITE_REG       0x20  //D��??????��??��,�̨�5???a??��??�¦�??��
#define RD_RX_PLOAD     0x61  //?��RX��DD�쨺y?Y,1~32��??��
#define WR_TX_PLOAD     0xA0  //D��TX��DD�쨺y?Y,1~32��??��
#define FLUSH_TX        0xE1  //??3yTX FIFO??��??��.���騦??�꨺???��?
#define FLUSH_RX        0xE2  //??3yRX FIFO??��??��.?����??�꨺???��?
#define REUSE_TX_PL     0xE3  //??D?��1��?��?��?�㨹��y?Y,CE?a??,��y?Y�㨹��?2???����?��.
#define NOP             0xFF  //??2������,?����?��?����?�����䨬???��??��	 
//SPI(NRF24L01)??��??�¦�??��
#define CONFIG          0x00  //??????��??�¦�??��;bit0:1?����??�꨺?,0���騦??�꨺?;bit1:��?????;bit2:CRC?�꨺?;bit3:CRC��1?��;
                              //bit4:?D??MAX_RT(��?��?��?�䨮??�����?��y?D??)��1?��;bit5:?D??TX_DS��1?��;bit6:?D??RX_DR��1?��
#define EN_AA           0x01  //��1?����??����|��e1|?��  bit0~5,??��|�����̨�0~5
#define EN_RXADDR       0x02  //?����?��??��?��D��,bit0~5,??��|�����̨�0~5
#define SETUP_AW        0x03  //����??��??��?��?��(?����D��y?Y�����̨�):bit1,0:00,3��??��;01,4��??��;02,5��??��;
#define SETUP_RETR      0x04  //?�������??��??����;bit3:0,��??��??����??��y?��;bit7:4,��??��??����?������ 250*x+86us
#define RF_CH           0x05  //RF�����̨�,bit6:0,1����¨����̨�?��?��;
#define RF_SETUP        0x06  //RF??��??��;bit3:��?��??��?��(0:1Mbps,1:2Mbps);bit2:1,���騦?1|?��;bit0:�̨�??������?�䨮?��??��?
#define STATUS          0x07  //���䨬???��??��;bit0:TX FIFO?������??;bit3:1,?����?��y?Y�����̨�o?(��?�䨮:6);bit4,��?��?��??����???����
                              //bit5:��y?Y����?������3��?D??;bit6:?����?��y?Y?D??;
#define MAX_TX  	0x10  //��?��?��?�䨮����?����?��y?D??
#define TX_OK   	0x20  //TX����?������3��?D??
#define RX_OK   	0x40  //?����?��?��y?Y?D??

#define OBSERVE_TX      0x08  //����?��?��2a??��??��,bit7:4,��y?Y�㨹?a����??��y?��;bit3:0,??����??��y?��
#define CD              0x09  //??2��?��2a??��??��,bit0,??2��?��2a;
#define RX_ADDR_P0      0x0A  //��y?Y�����̨�0?����?��??��,��?�䨮3��?��5??��??��,�̨���??��?��?��
#define RX_ADDR_P1      0x0B  //��y?Y�����̨�1?����?��??��,��?�䨮3��?��5??��??��,�̨���??��?��?��
#define RX_ADDR_P2      0x0C  //��y?Y�����̨�2?����?��??��,��?�̨���??��?������??,??��??��,��?D?��?RX_ADDR_P1[39:8]?���̨�;
#define RX_ADDR_P3      0x0D  //��y?Y�����̨�3?����?��??��,��?�̨���??��?������??,??��??��,��?D?��?RX_ADDR_P1[39:8]?���̨�;
#define RX_ADDR_P4      0x0E  //��y?Y�����̨�4?����?��??��,��?�̨���??��?������??,??��??��,��?D?��?RX_ADDR_P1[39:8]?���̨�;
#define RX_ADDR_P5      0x0F  //��y?Y�����̨�5?����?��??��,��?�̨���??��?������??,??��??��,��?D?��?RX_ADDR_P1[39:8]?���̨�;
#define TX_ADDR         0x10  //����?����??��(�̨���??��?��?��),ShockBurstTM?�꨺???,RX_ADDR_P0��?��?��??��?���̨�
#define RX_PW_P0        0x11  //?����?��y?Y�����̨�0��DD�쨺y?Y?��?��(1~32��??��),����???a0?����?����
#define RX_PW_P1        0x12  //?����?��y?Y�����̨�1��DD�쨺y?Y?��?��(1~32��??��),����???a0?����?����
#define RX_PW_P2        0x13  //?����?��y?Y�����̨�2��DD�쨺y?Y?��?��(1~32��??��),����???a0?����?����
#define RX_PW_P3        0x14  //?����?��y?Y�����̨�3��DD�쨺y?Y?��?��(1~32��??��),����???a0?����?����
#define RX_PW_P4        0x15  //?����?��y?Y�����̨�4��DD�쨺y?Y?��?��(1~32��??��),����???a0?����?����
#define RX_PW_P5        0x16  //?����?��y?Y�����̨�5��DD�쨺y?Y?��?��(1~32��??��),����???a0?����?����
#define FIFO_STATUS     0x17  //FIFO���䨬???��??��;bit0,RX FIFO??��??��??����??;bit1,RX FIFO?������??;bit2,3,���ꨢ?
                              //bit4,TX FIFO??����??;bit5,TX FIFO?������??;bit6,1,?-?������?����?��?��y?Y�㨹.0,2??-?��;
//////////////////////////////////////////////////////////////////////////////////////////////////////////

const uint8_t TX_ADDRESS[TX_ADR_WIDTH]={0x34,0x43,0x10,0x10}; //����?����??��
const uint8_t RX_ADDRESS[RX_ADR_WIDTH]={0x34,0x43,0x10,0x10}; //����?����??��

static uint8_t nrf24l01ReadReg(uint8_t reg)
{
	uint8_t val;
  	NRF24L01_CSN_LOW();          
  	HalSpiReadWrite(reg);
  	val = HalSpiReadWrite(0xff);     
  	NRF24L01_CSN_HIGH();       
  	return val;           
}

static uint8_t nrf24l01WriteReg(uint8_t reg, uint8_t value)
{
	uint8_t status;	
   	NRF24L01_CSN_LOW();                 
  	status = HalSpiReadWrite(reg);
  	HalSpiReadWrite(value);      
  	NRF24L01_CSN_HIGH();   
  	return status;
}

static uint8_t nrf24l01ReadBuf(uint8_t reg, uint8_t *pBuf, uint8_t len)
{
	uint8_t status, i;	       
  	NRF24L01_CSN_LOW();          
  	status = HalSpiReadWrite(reg);
 	for(i = 0; i < len; i++)
    {
        pBuf[i] = HalSpiReadWrite(0xFF);
    }
  	NRF24L01_CSN_HIGH();
  	return status;
}

static uint8_t nrf24l01WriteBuf(uint8_t reg, const uint8_t *pBuf, uint8_t len)
{
	uint8_t status, i;	    
 	NRF24L01_CSN_LOW(); 
  	status = HalSpiReadWrite(reg);
  	for(i = 0; i < len; i++)
    {
        HalSpiReadWrite(pBuf[i]);
    }
  	NRF24L01_CSN_HIGH();
  	return status;
}

uint8_t NRF24L01TxPacket(const uint8_t *txbuf, uint8_t len)
{
	uint8_t sta;
	NRF24L01_CE_LOW();
    
    HalInterruptsSetEnable(false);
  	nrf24l01WriteBuf(WR_TX_PLOAD, txbuf, len);
 	NRF24L01_CE_HIGH();
	while(HalGPIOGet(NRF24L01_IRQ_PIN) != 0);
	sta = nrf24l01ReadReg(STATUS);
	nrf24l01WriteReg(WRITE_REG+STATUS,sta);
    
    nrf24l01WriteReg(FLUSH_RX, 0xff);
    
    HalInterruptsSetEnable(true);
	if(sta & MAX_TX)
	{
		nrf24l01WriteReg(FLUSH_TX, 0xff);
		return MAX_TX; 
	}
	if(sta & TX_OK)
	{
		return TX_OK;
	}
    
	return 0xff;
}

uint8_t NRF24L01RxPacket(uint8_t *rxbuf)
{
	uint8_t sta;		    							   

	sta = nrf24l01ReadReg(STATUS);
	nrf24l01WriteReg(WRITE_REG + STATUS, sta);
	if(sta & RX_OK)
	{
		nrf24l01ReadBuf(RD_RX_PLOAD, rxbuf, RX_PLOAD_WIDTH);
		nrf24l01WriteReg(FLUSH_RX, 0xff);
        nrf24l01WriteReg(FLUSH_TX, 0xff);
		return 0; 
	}	   
	return 1;
}

bool NRF24L01Check(void)
{
	uint8_t buf[5]={0XA5,0XA5,0XA5,0XA5,0XA5};
	uint8_t i;
    SysLog("");

	nrf24l01WriteBuf(WRITE_REG + TX_ADDR, buf, 5);
    memset(buf, 0xff, sizeof(buf));
	nrf24l01ReadBuf(TX_ADDR, buf, 5);
    //HalPrintf("RECV:%02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
	for(i = 0; i < 5; i++)
    {
        if(buf[i] != 0XA5)
        {
            return false;
        }
    }   

	return true;
}

void NRF24l01Shutdown(void)
{
    NRF24L01_CE_LOW();
    nrf24l01WriteReg(WRITE_REG+CONFIG, 0x0D);
    NRF24L01_CE_HIGH();
    hal_wait_us(30);
}

void NRF24L01ModeSet(NRF24L01Mode_t mode, uint8_t chn)
{
    NRF24l01Shutdown();
    if(mode == NRF24L01RecvMode)
    {
        NRF24L01_CE_LOW();	  
  	    nrf24l01WriteBuf(WRITE_REG+RX_ADDR_P0, (uint8_t *)RX_ADDRESS, RX_ADR_WIDTH);//дRX�ڵ��ַ
  	    nrf24l01WriteReg(WRITE_REG+SETUP_AW, 0x02);
  	    nrf24l01WriteReg(WRITE_REG+EN_AA, 0x00);
        nrf24l01WriteReg(WRITE_REG+EN_RXADDR, 0x01);//ʹ��ͨ��0�Ľ��յ�ַ  	
        nrf24l01WriteReg(WRITE_REG+SETUP_RETR, 0x00);//
        nrf24l01WriteReg(WRITE_REG+RF_CH, chn);	     //����RFͨ��Ƶ��	
        nrf24l01WriteReg(WRITE_REG+RX_PW_P0, RX_PLOAD_WIDTH);//ѡ��ͨ��0����Ч���ݿ�� 	 
        nrf24l01WriteReg(WRITE_REG+RF_SETUP, 0x27);//����TX�������,0db����,250Kbps,���������濪�� 
        nrf24l01WriteReg(WRITE_REG+CONFIG, 0x0b);//���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,8BIT_CRC,����ģʽ 
        NRF24L01_CE_HIGH(); //CEΪ��,�������ģʽ 
    }
    else
    {
        NRF24L01_CE_LOW();	    
      	nrf24l01WriteBuf(WRITE_REG+TX_ADDR, (uint8_t *)TX_ADDRESS, TX_ADR_WIDTH);//дTX�ڵ��ַ 
      	nrf24l01WriteBuf(WRITE_REG+RX_ADDR_P0, (uint8_t *)RX_ADDRESS, RX_ADR_WIDTH); //����TX�ڵ��ַ,��ҪΪ��ʹ��ACK	 
      	nrf24l01WriteReg(WRITE_REG+SETUP_AW, 0x02);

      	nrf24l01WriteReg(WRITE_REG+EN_AA, 0x00);     //��ʹ��ͨ��0���Զ�Ӧ��    
      	nrf24l01WriteReg(WRITE_REG+EN_RXADDR, 0x01); //ʹ��ͨ��0�Ľ��յ�ַ  
      	nrf24l01WriteReg(WRITE_REG+SETUP_RETR, 0x00);//
      	nrf24l01WriteReg(WRITE_REG+RF_CH, chn);       //����RFͨ��Ϊ
      	nrf24l01WriteReg(WRITE_REG+RF_SETUP, 0x27);  //����TX�������,0db����,250KMbps,���������濪��   
      	nrf24l01WriteReg(WRITE_REG+CONFIG, 0x0a);    //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,8BIT_CRC,����ģʽ,���������ж�
    	NRF24L01_CE_HIGH();//CEΪ��,10us����������
    }
    hal_wait_us(150);
}

uint8_t NRF24L01GetStatus(void)
{
    return nrf24l01ReadReg(FIFO_STATUS);
}

bool NRF24L01DataRecved(void)
{
    return HalGPIOGet(NRF24L01_IRQ_PIN) == HAL_GPIO_LEVEL_LOW;
}
#if 0
static void nrf24l01SetPinIRQ(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;  
    NVIC_InitTypeDef NVIC_InitStructure;  
        
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource1);
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;    //��������  
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; //����Ϊ�ⲿ�ж�ģʽ  
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // �����������½��ض����  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;                //ʹ���ⲿ�ж���״̬  
    EXTI_Init(&EXTI_InitStructure); 
    
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                //ʹ���ⲿ�ж�ͨ��     
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_1_IRQn;  
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x00;         //��ռ���ȼ�4λ,��16��  
    NVIC_Init(&NVIC_InitStructure);   //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���  
}
#endif

void NRF24L01Sleep(bool sleep)
{
    if(sleep)
    {
        NRF24l01Shutdown();
    }
    else
    {
        NRF24L01_CE_LOW();
        nrf24l01WriteReg(WRITE_REG+CONFIG, 0x0b);
        NRF24L01_CE_HIGH();
        hal_wait_ms(2);
    }
}

void NRF24L01Initialize(void)
{
    HalGPIOInit(NRF24L01_CE_PIN, HAL_GPIO_DIR_OUT);
    HalGPIOInit(NRF24L01_CSN_PIN, HAL_GPIO_DIR_OUT);
    
    HalGPIOInit(NRF24L01_IRQ_PIN, HAL_GPIO_DIR_IN);
    //nrf24l01SetPinIRQ();
    
    NRF24L01_CE_LOW();
    NRF24L01_CSN_HIGH();
}

void NRF24L01Poll(void)
{

}

