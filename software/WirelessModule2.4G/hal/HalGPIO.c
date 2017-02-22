#include "HalGPIO.h"

#define IO_MAX   48

typedef struct halGpio{
	GPIO_TypeDef *port;
	uint16_t pin;
	uint32_t rcc; //¨º¡À?¨®
	uint8_t  intPort;
	uint8_t  intPin;
	uint32_t extiLine;
	
}HALGPIO;


static void numToGPIO(HalGPIO_t num, HALGPIO *gpio)
{
	HalGPIO_t port;
	port           = (num>>4)&0x0f;
	gpio->pin      = (uint16_t)(1<<(num&0x0f));  //eg. GPIO_Pin_11=(uint16_t)0x0800
	gpio->extiLine = (uint32_t)(1<<(num&0x0f));  //eg. EXTI_Line3 = (uint32_t)0x00008
	gpio->intPin   = (uint8_t)(num&0x0f);        //eg. GPIO_PinSource1=(uint8_t)0x01
	switch(port)
	{
		case 0:
			gpio->port = GPIOA;
			gpio->rcc  = RCC_AHBPeriph_GPIOA;
//			gpio->intPort = GPIO_PortSourceGPIOA;
			break;
		case 1:
			gpio->port = GPIOB;
			gpio->rcc  = RCC_AHBPeriph_GPIOB;
//			gpio->intPort = GPIO_PortSourceGPIOB;
			break;
		case 2:
			gpio->port = GPIOC;
			gpio->rcc  = RCC_AHBPeriph_GPIOC;
//			gpio->intPort = GPIO_PortSourceGPIOC;
			break;
		case 3:
			gpio->port = GPIOD;
			gpio->rcc  = RCC_AHBPeriph_GPIOD;
//			gpio->intPort = GPIO_PortSourceGPIOD;
			break;
		case 4:
			gpio->port = GPIOE;
			gpio->rcc  = RCC_AHBPeriph_GPIOE;
//			gpio->intPort = GPIO_PortSourceGPIOE;
			break;
		case 5:
			gpio->port = GPIOF;
			gpio->rcc  = RCC_AHBPeriph_GPIOF;
//			gpio->intPort = GPIO_PortSourceGPIOF;
			break;
		default:
			break;
	}
}



void HalGPIOPoll(void)
{

}


void HalGPIOSet(HalGPIO_t io, HalGPIOLevel_t value)
{
	HALGPIO gpio;

    if(io > IO_MAX)
        return;
	
	numToGPIO(io, &gpio);
	GPIO_WriteBit(gpio.port, gpio.pin, (BitAction)value);
}

#define GPIO_UART_PIN_NUM   12

void HalGpioUartTxSet(uint8_t value)
{
	HALGPIO gpio;
	uint16_t regVal;

	regVal = GPIO_ReadOutputData(gpio.port);
	if(value)
	{
		regVal |= 0x0001<<GPIO_UART_PIN_NUM;
	}else{
		regVal &= ~(0x0001<<GPIO_UART_PIN_NUM);
	}
	GPIO_Write(GPIOB, regVal);
}

HalGPIOLevel_t HalGPIOGet(HalGPIO_t io)
{
	HALGPIO gpio;
    
    if(io > IO_MAX)
        return 0;
    
	numToGPIO(io, &gpio);

	return (HalGPIOLevel_t)GPIO_ReadInputDataBit(gpio.port, gpio.pin);
}

void HalGPIOInit(HalGPIO_t num, const HalGPIODir_t dir)
{
	HALGPIO gpio;
	GPIO_InitTypeDef gpioStruct;

    if(num > IO_MAX)
        return;
	
	numToGPIO(num, &gpio);
	
	gpioStruct.GPIO_Pin   = gpio.pin;
	gpioStruct.GPIO_Speed = GPIO_Speed_50MHz;
    
	switch(dir)
	{
		
		case HAL_GPIO_DIR_IN:
			gpioStruct.GPIO_Mode  = GPIO_Mode_IN;
			break;
    	case HAL_GPIO_DIR_OUT:
			gpioStruct.GPIO_Mode  = GPIO_Mode_OUT;
            gpioStruct.GPIO_PuPd = GPIO_PuPd_UP;
            gpioStruct.GPIO_OType = GPIO_OType_PP;
			break;
        default: //interrupt
			gpioStruct.GPIO_Mode  = GPIO_Mode_AF;
			break;
	}
	
	GPIO_Init(gpio.port, &gpioStruct);
}


void HalGPIOInitialize(void)
{

}


