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
//NRF24L01??′??÷2ù×÷?üá?
#define READ_REG        0x00  //?á??????′??÷,μí5???a??′??÷μ??·
#define WRITE_REG       0x20  //D′??????′??÷,μí5???a??′??÷μ??·
#define RD_RX_PLOAD     0x61  //?áRXóDD§êy?Y,1~32×??ú
#define WR_TX_PLOAD     0xA0  //D′TXóDD§êy?Y,1~32×??ú
#define FLUSH_TX        0xE1  //??3yTX FIFO??′??÷.·￠é??￡ê???ó?
#define FLUSH_RX        0xE2  //??3yRX FIFO??′??÷.?óê??￡ê???ó?
#define REUSE_TX_PL     0xE3  //??D?ê1ó?é?ò?°üêy?Y,CE?a??,êy?Y°ü±?2???·￠?í.
#define NOP             0xFF  //??2ù×÷,?éò?ó?à′?á×′ì???′??÷	 
//SPI(NRF24L01)??′??÷μ??·
#define CONFIG          0x00  //??????′??÷μ??·;bit0:1?óê??￡ê?,0·￠é??￡ê?;bit1:μ?????;bit2:CRC?￡ê?;bit3:CRCê1?ü;
                              //bit4:?D??MAX_RT(′?μ?×?′ó??·￠′?êy?D??)ê1?ü;bit5:?D??TX_DSê1?ü;bit6:?D??RX_DRê1?ü
#define EN_AA           0x01  //ê1?ü×??ˉó|′e1|?ü  bit0~5,??ó|í¨μà0~5
#define EN_RXADDR       0x02  //?óê?μ??·?êDí,bit0~5,??ó|í¨μà0~5
#define SETUP_AW        0x03  //éè??μ??·?í?è(?ùóDêy?Yí¨μà):bit1,0:00,3×??ú;01,4×??ú;02,5×??ú;
#define SETUP_RETR      0x04  //?¨á￠×??ˉ??·￠;bit3:0,×??ˉ??·￠??êy?÷;bit7:4,×??ˉ??·￠?óê± 250*x+86us
#define RF_CH           0x05  //RFí¨μà,bit6:0,1¤×÷í¨μà?μ?ê;
#define RF_SETUP        0x06  //RF??′??÷;bit3:′?ê??ù?ê(0:1Mbps,1:2Mbps);bit2:1,·￠é?1|?ê;bit0:μí??éù·?′ó?÷??ò?
#define STATUS          0x07  //×′ì???′??÷;bit0:TX FIFO?ú±ê??;bit3:1,?óê?êy?Yí¨μào?(×?′ó:6);bit4,′?μ?×??à′???·￠
                              //bit5:êy?Y·￠?ííê3é?D??;bit6:?óê?êy?Y?D??;
#define MAX_TX  	0x10  //′?μ?×?′ó·￠?í′?êy?D??
#define TX_OK   	0x20  //TX·￠?ííê3é?D??
#define RX_OK   	0x40  //?óê?μ?êy?Y?D??

#define OBSERVE_TX      0x08  //·￠?í?ì2a??′??÷,bit7:4,êy?Y°ü?aê§??êy?÷;bit3:0,??·￠??êy?÷
#define CD              0x09  //??2¨?ì2a??′??÷,bit0,??2¨?ì2a;
#define RX_ADDR_P0      0x0A  //êy?Yí¨μà0?óê?μ??·,×?′ó3¤?è5??×??ú,μí×??ú?ú?°
#define RX_ADDR_P1      0x0B  //êy?Yí¨μà1?óê?μ??·,×?′ó3¤?è5??×??ú,μí×??ú?ú?°
#define RX_ADDR_P2      0x0C  //êy?Yí¨μà2?óê?μ??·,×?μí×??ú?ééè??,??×??ú,±?D?í?RX_ADDR_P1[39:8]?àμè;
#define RX_ADDR_P3      0x0D  //êy?Yí¨μà3?óê?μ??·,×?μí×??ú?ééè??,??×??ú,±?D?í?RX_ADDR_P1[39:8]?àμè;
#define RX_ADDR_P4      0x0E  //êy?Yí¨μà4?óê?μ??·,×?μí×??ú?ééè??,??×??ú,±?D?í?RX_ADDR_P1[39:8]?àμè;
#define RX_ADDR_P5      0x0F  //êy?Yí¨μà5?óê?μ??·,×?μí×??ú?ééè??,??×??ú,±?D?í?RX_ADDR_P1[39:8]?àμè;
#define TX_ADDR         0x10  //·￠?íμ??·(μí×??ú?ú?°),ShockBurstTM?￡ê???,RX_ADDR_P0ó?′?μ??·?àμè
#define RX_PW_P0        0x11  //?óê?êy?Yí¨μà0óDD§êy?Y?í?è(1~32×??ú),éè???a0?ò·?·¨
#define RX_PW_P1        0x12  //?óê?êy?Yí¨μà1óDD§êy?Y?í?è(1~32×??ú),éè???a0?ò·?·¨
#define RX_PW_P2        0x13  //?óê?êy?Yí¨μà2óDD§êy?Y?í?è(1~32×??ú),éè???a0?ò·?·¨
#define RX_PW_P3        0x14  //?óê?êy?Yí¨μà3óDD§êy?Y?í?è(1~32×??ú),éè???a0?ò·?·¨
#define RX_PW_P4        0x15  //?óê?êy?Yí¨μà4óDD§êy?Y?í?è(1~32×??ú),éè???a0?ò·?·¨
#define RX_PW_P5        0x16  //?óê?êy?Yí¨μà5óDD§êy?Y?í?è(1~32×??ú),éè???a0?ò·?·¨
#define FIFO_STATUS     0x17  //FIFO×′ì???′??÷;bit0,RX FIFO??′??÷??±ê??;bit1,RX FIFO?ú±ê??;bit2,3,±￡á?
                              //bit4,TX FIFO??±ê??;bit5,TX FIFO?ú±ê??;bit6,1,?-?··￠?íé?ò?êy?Y°ü.0,2??-?·;
//////////////////////////////////////////////////////////////////////////////////////////////////////////

const uint8_t TX_ADDRESS[TX_ADR_WIDTH]={0x34,0x43,0x10,0x10}; //·￠?íμ??·
const uint8_t RX_ADDRESS[RX_ADR_WIDTH]={0x34,0x43,0x10,0x10}; //·￠?íμ??·

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
  	    nrf24l01WriteBuf(WRITE_REG+RX_ADDR_P0, (uint8_t *)RX_ADDRESS, RX_ADR_WIDTH);//写RX节点地址
  	    nrf24l01WriteReg(WRITE_REG+SETUP_AW, 0x02);
  	    nrf24l01WriteReg(WRITE_REG+EN_AA, 0x00);
        nrf24l01WriteReg(WRITE_REG+EN_RXADDR, 0x01);//使能通道0的接收地址  	
        nrf24l01WriteReg(WRITE_REG+SETUP_RETR, 0x00);//
        nrf24l01WriteReg(WRITE_REG+RF_CH, chn);	     //设置RF通信频率	
        nrf24l01WriteReg(WRITE_REG+RX_PW_P0, RX_PLOAD_WIDTH);//选择通道0的有效数据宽度 	 
        nrf24l01WriteReg(WRITE_REG+RF_SETUP, 0x27);//设置TX发射参数,0db增益,250Kbps,低噪声增益开启 
        nrf24l01WriteReg(WRITE_REG+CONFIG, 0x0b);//配置基本工作模式的参数;PWR_UP,EN_CRC,8BIT_CRC,接收模式 
        NRF24L01_CE_HIGH(); //CE为高,进入接收模式 
    }
    else
    {
        NRF24L01_CE_LOW();	    
      	nrf24l01WriteBuf(WRITE_REG+TX_ADDR, (uint8_t *)TX_ADDRESS, TX_ADR_WIDTH);//写TX节点地址 
      	nrf24l01WriteBuf(WRITE_REG+RX_ADDR_P0, (uint8_t *)RX_ADDRESS, RX_ADR_WIDTH); //设置TX节点地址,主要为了使能ACK	 
      	nrf24l01WriteReg(WRITE_REG+SETUP_AW, 0x02);

      	nrf24l01WriteReg(WRITE_REG+EN_AA, 0x00);     //不使能通道0的自动应答    
      	nrf24l01WriteReg(WRITE_REG+EN_RXADDR, 0x01); //使能通道0的接收地址  
      	nrf24l01WriteReg(WRITE_REG+SETUP_RETR, 0x00);//
      	nrf24l01WriteReg(WRITE_REG+RF_CH, chn);       //设置RF通道为
      	nrf24l01WriteReg(WRITE_REG+RF_SETUP, 0x27);  //设置TX发射参数,0db增益,250KMbps,低噪声增益开启   
      	nrf24l01WriteReg(WRITE_REG+CONFIG, 0x0a);    //配置基本工作模式的参数;PWR_UP,EN_CRC,8BIT_CRC,接收模式,开启所有中断
    	NRF24L01_CE_HIGH();//CE为高,10us后启动发送
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
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;    //设置引脚  
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; //设置为外部中断模式  
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // 设置上升和下降沿都检测  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;                //使能外部中断新状态  
    EXTI_Init(&EXTI_InitStructure); 
    
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                //使能外部中断通道     
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_1_IRQn;  
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x00;         //先占优先级4位,共16级  
    NVIC_Init(&NVIC_InitStructure);   //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器  
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

